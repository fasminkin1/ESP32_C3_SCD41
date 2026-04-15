#include "ble_mgr.h"
#include "app_config.h"
#include "sensors.h"
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(ble_mgr, LOG_LEVEL_INF);

/* Custom Service UUID: SCD41 Air Monitor (12345678-abcd-4444-8888-534344343100) */
#define BT_UUID_SCD_SERVICE_VAL BT_UUID_128_ENCODE(0x12345678, 0xabcd, 0x4444, 0x8888, 0x534344343100)
/* Characteristic: Telemetry (Notify) */
#define BT_UUID_SCD_DATA_VAL    BT_UUID_128_ENCODE(0x12345678, 0xabcd, 0x4444, 0x8888, 0x534344343101)
/* Characteristic: Command (Write) */
#define BT_UUID_SCD_CMD_VAL     BT_UUID_128_ENCODE(0x12345678, 0xabcd, 0x4444, 0x8888, 0x534344343102)

static struct bt_uuid_128 scd_svc_uuid = BT_UUID_INIT_128(BT_UUID_SCD_SERVICE_VAL);
static struct bt_uuid_128 scd_data_uuid = BT_UUID_INIT_128(BT_UUID_SCD_DATA_VAL);
static struct bt_uuid_128 scd_cmd_uuid = BT_UUID_INIT_128(BT_UUID_SCD_CMD_VAL);

static uint8_t telemetry_buf[128];
static struct bt_conn *current_conn;

/* ---- Characteristic Handlers ---- */
static ssize_t read_telemetry(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, telemetry_buf, strlen(telemetry_buf));
}

static ssize_t write_command(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	char cmd_buf[64];
	uint16_t copy_len = MIN(len, sizeof(cmd_buf) - 1);
	
	memcpy(cmd_buf, buf, copy_len);
	cmd_buf[copy_len] = '\0';

	LOG_INF("BLE Command received: %s", cmd_buf);

	/* Execute command via Shell */
	shell_execute_cmd(NULL, cmd_buf);

	/* Trigger immediate telemetry update to sync app state */
	ble_mgr_send_telemetry();

	return len;
}

static void ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Telemetry notifications %s", notif_enabled ? "enabled" : "disabled");
}

/* ---- GATT Definition ---- */
BT_GATT_SERVICE_DEFINE(scd_svc,
	BT_GATT_PRIMARY_SERVICE(&scd_svc_uuid),
	BT_GATT_CHARACTERISTIC(&scd_data_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_telemetry, NULL, telemetry_buf),
	BT_GATT_CCC(ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	BT_GATT_CHARACTERISTIC(&scd_cmd_uuid.uuid,
			       BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
			       BT_GATT_PERM_WRITE,
			       NULL, write_command, NULL),
);

static uint8_t ad_service_data[4]; /* 2 bytes UUID (0x18, 0x1A) + 2 bytes CO2 */

static struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_SCD_SERVICE_VAL),
	BT_DATA(BT_DATA_SVC_DATA16, ad_service_data, sizeof(ad_service_data)),
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void update_ad_data(void)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	uint16_t co2 = (uint16_t)last_data.co2;
	k_mutex_unlock(&config_mutex);

	/* UUID 0x181A (Environmental Sensing) in Little Endian */
	ad_service_data[0] = 0x1A;
	ad_service_data[1] = 0x18;
	/* CO2 value in Big Endian for easier parsing on Android */
	ad_service_data[2] = (co2 >> 8) & 0xFF;
	ad_service_data[3] = co2 & 0xFF;

	bt_le_adv_update_data(ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
}

/* ---- BLE Callbacks ---- */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}
	LOG_INF("Phone connected");
	current_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Phone disconnected (reason %02x)", reason);
	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}

	/* Restart advertising so the device is visible again without power cycle */
	int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to restart (err %d)", err);
	} else {
		LOG_INF("Advertising restarted");
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(int err)
{
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return;
	}
	LOG_INF("Bluetooth initialized and ready");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return;
	}
	LOG_INF("Advertising started as 'SCD41_Monitor'");
}

int ble_mgr_init(void)
{
	int err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
	}
	return err;
}

void ble_mgr_send_telemetry(void)
{
	sensors_build_telemetry_json(telemetry_buf, sizeof(telemetry_buf));

	/* Notify index 2 (The value attribute) */
	if (current_conn) {
		bt_gatt_notify(NULL, &scd_svc.attrs[2], telemetry_buf, strlen(telemetry_buf));
	}
	
	/* Update advertising data so CO2 is visible without connection */
	update_ad_data();
}

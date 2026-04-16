#ifndef BLE_MGR_H
#define BLE_MGR_H

#include <zephyr/kernel.h>

enum ble_status {
	BLE_STATUS_DISCONNECTED,
	BLE_STATUS_CONNECTING,
	BLE_STATUS_CONNECTED,
	BLE_STATUS_DISCONNECTING
};

/**
 * @brief Initialize BLE service and advertising
 * @return 0 on success
 */
int ble_mgr_init(void);

/**
 * @brief Update and notify telemetry data over BLE
 */
void ble_mgr_send_telemetry(void);

/**
 * @brief Get current BLE connection status
 */
enum ble_status ble_mgr_get_status(void);

#endif

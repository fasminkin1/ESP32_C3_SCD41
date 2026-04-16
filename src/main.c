#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "app_config.h"
#include "sensors.h"
#include "display_mgr.h"
#include "shell_cmds.h"
#include "ble_mgr.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

int main(void)
{
	int err;

	LOG_INF("Initializing SCD41 Air Monitor with BLE");

	err = app_config_init();
	if (err) LOG_ERR("Settings initialization failed: %d", err);

	err = sensors_init();
	if (err) {
		LOG_ERR("Sensor initialization failed: %d", err);
		return 0;
	}

	err = display_mgr_init();
	if (err) {
		LOG_ERR("Display initialization failed: %d", err);
		return 0;
	}

	display_mgr_show_splash();

	/* Initialize Bluetooth via dedicated manager */
	err = ble_mgr_init();
	if (err) {
		LOG_ERR("BLE initialization failed: %d", err);
	}

	shell_cmds_init();

	LOG_INF("System ready. Bluetooth advertising...");

	int64_t last_timer_tick = k_uptime_get();
	int64_t last_sensor_fetch = 0;

	while (1) {
		uint32_t conf_interval_ms;
		bool is_active;
		
		k_mutex_lock(&config_mutex, K_FOREVER);
		is_active = config.active;
		conf_interval_ms = config.interval_ms;
		
		/* Handle Timer Countdown */
		if (config.timer_min > 0) {
			int64_t now = k_uptime_get();
			if (now - last_timer_tick >= 60000) { /* 1 minute passed */
				config.timer_min--;
				last_timer_tick = now;
				LOG_INF("Timer: %d minutes remaining", config.timer_min);
				
				if (config.timer_min == 0) {
					config.active = false;
					is_active = false;
					LOG_INF("Timer expired! Stopping measurements.");
				}
			}
		} else {
			last_timer_tick = k_uptime_get();
		}
		k_mutex_unlock(&config_mutex);

		int64_t now = k_uptime_get();
		if (is_active && (now - last_sensor_fetch >= conf_interval_ms || last_sensor_fetch == 0)) {
			err = sensors_fetch_all();
			if (err == 0) {
				ble_mgr_send_telemetry();
			} else {
				LOG_ERR("Sensor fetch failed: %d", err);
			}
			last_sensor_fetch = now;
		}

		display_mgr_update();
		k_msleep(200); /* 5 FPS UI update for animations */
	}

	return 0;
}

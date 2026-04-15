#ifndef BLE_MGR_H
#define BLE_MGR_H

#include <zephyr/kernel.h>

/**
 * @brief Initialize BLE service and advertising
 * @return 0 on success
 */
int ble_mgr_init(void);

/**
 * @brief Update and notify telemetry data over BLE
 */
void ble_mgr_send_telemetry(void);

#endif

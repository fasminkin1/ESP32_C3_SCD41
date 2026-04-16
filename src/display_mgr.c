#include "display_mgr.h"
#include "app_config.h"
#include "sensors.h"
#include "ble_mgr.h"
#include <zephyr/logging/log.h>
#include <zephyr/display/cfb.h>
#include <zephyr/drivers/display.h>
#include <stdio.h>

LOG_MODULE_DECLARE(app, LOG_LEVEL_INF);

const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int display_mgr_init(void)
{
	if (!device_is_ready(display)) {
		return -ENODEV;
	}
	if (display_set_pixel_format(display, PIXEL_FORMAT_MONO10) < 0) {
		return -EIO;
	}
	if (cfb_framebuffer_init(display)) {
		return -EIO;
	}

	cfb_framebuffer_clear(display, true);
	cfb_framebuffer_set_font(display, 0);

	display_blanking_off(display);

	return 0;
}

void display_mgr_show_splash(void)
{
	uint16_t width, height;
	uint8_t rows, cols;

	width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);
	rows = cfb_get_display_parameter(display, CFB_DISPLAY_ROWS);
	cols = cfb_get_display_parameter(display, CFB_DISPLAY_COLS);

	LOG_INF("Display: %dx%d, %d rows, %d cols", width, height, rows, cols);

	int num_fonts = cfb_get_numof_fonts(display);
	LOG_INF("Fonts available: %d", num_fonts);

	for (int i = 0; i < num_fonts; i++) {
		uint8_t f_width, f_height;
		cfb_get_font_size(display, i, &f_width, &f_height);
		LOG_INF("Font %d: %dx%d", i, f_width, f_height);
	}

	LOG_INF("Showing splash screen (version %s)", APP_VERSION);
	cfb_framebuffer_clear(display, false);
	cfb_print(display, "SCD41", 0, 0);
	cfb_print(display, (char *)APP_VERSION, 0, 20);
	cfb_framebuffer_finalize(display);
	k_sleep(K_SECONDS(5));
	LOG_INF("Splash screen finished");
}

void display_mgr_update(void)
{
	static uint8_t page_idx = 0;
	char val_buf[32];
	const char *label = "";
	bool shown = false;
	bool ru = config.lang_ru;
	enum ble_status ble_st = ble_mgr_get_status();
	const char *ble_str = (ble_st == BLE_STATUS_CONNECTED)    ? "BT:OK"
			      : (ble_st == BLE_STATUS_CONNECTING) ? "BT:.."
								  : "BT:--";

	k_mutex_lock(&config_mutex, K_FOREVER);

	/* Total pages: 5 metrics + 1 sensor status + 1 BLE status = 7 */
	for (int i = 0; i < 7; i++) {
		uint8_t current = (page_idx + i) % 7;

		cfb_framebuffer_clear(display, false);

		if (current < 5) {
			/* Metrics pages (Compact style) */
			switch (current) {
			case 0:
				if (config.measure_co2) {
					label = "CO2|ppm";
					snprintf(val_buf, sizeof(val_buf), "%d", last_data.co2);
					shown = true;
				}
				break;
			case 1:
				if (config.measure_temp) {
					label = "t C";
					snprintf(val_buf, sizeof(val_buf), "%.2f", last_data.temp);
					shown = true;
				}
				break;
			case 2:
				if (config.measure_hum) {
					label = ru ? "Вл %" : "Hum %";
					snprintf(val_buf, sizeof(val_buf), "%.1f", last_data.hum);
					shown = true;
				}
				break;
			case 3:
				if (config.measure_dp) {
					label = ru ? "Т.Рос|C" : "DewP|C";
					snprintf(val_buf, sizeof(val_buf), "%.1f", last_data.dp);
					shown = true;
				}
				break;
			case 4:
				if (config.measure_vpd) {
					label = "VPD|kPa";
					snprintf(val_buf, sizeof(val_buf), "%.2f", last_data.vpd);
					shown = true;
				}
				break;
			}

			if (shown) {
				/* Back to standard coordinates */
				cfb_print(display, (char *)label, 0, 0);
				cfb_print(display, val_buf, 0, 20);
			}
		} else if (current == 5) {
			/* Status screen Page */
			const char *status = sensors_get_status_word();
			cfb_print(display, ru ? "ВОЗД:" : "AIR:", 0, 0);
			cfb_print(display, (char *)status, 0, 20);
			shown = true;
		} else if (current == 6) {
			/* BLE Status Page */
			const char *st_desc = (ble_st == BLE_STATUS_CONNECTED)    ? (ru ? "СВЯЗЬ: OK" : "LINK: OK")
					      : (ble_st == BLE_STATUS_CONNECTING) ? (ru ? "ПОИСК.." : "WAIT..")
									  : (ru ? "ОТКЛЮЧ" : "DISCONN");
			cfb_print(display, "BLE:", 0, 0);
			cfb_print(display, (char *)st_desc, 0, 20);
			shown = true;
		}

		if (shown) {
			cfb_framebuffer_finalize(display);
			page_idx = (current + 1) % 7;
			break;
		}
	}

	k_mutex_unlock(&config_mutex);
}

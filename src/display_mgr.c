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

	width = cfb_get_display_parameter(display, CFB_DISPLAY_WIDTH);
	height = cfb_get_display_parameter(display, CFB_DISPLAY_HEIGHT);

	LOG_INF("Showing splash screen (version %s)", APP_VERSION);

	for (int i = 0; i < 20; i++) {
		cfb_framebuffer_clear(display, false);
		cfb_print(display, "SCD41 Monitor", 0, 0);
		cfb_print(display, (char *)APP_VERSION, 0, 20);

		char loading_str[32];
		snprintf(loading_str, sizeof(loading_str), "Loading%s",
		         (i % 4 == 0) ? "" : (i % 4 == 1) ? "." : (i % 4 == 2) ? ".." : "...");

		/* Simple scroll animation from right to left */
		int x_offset = width - (i * 8);
		if (x_offset < 0) x_offset = 0;

		cfb_print(display, loading_str, x_offset, 40);
		cfb_framebuffer_finalize(display);
		k_sleep(K_MSEC(150));
	}
	LOG_INF("Splash screen finished");
}

void display_mgr_update(void)
{
	static uint8_t page_idx = 0;
	static int64_t last_page_time = 0;
	static uint32_t anim_tick = 0;

	char val_buf[32];
	const char *label = "";
	bool shown = false;
	
	k_mutex_lock(&config_mutex, K_FOREVER);
	bool ru = config.lang_ru;
	k_mutex_unlock(&config_mutex);

	enum ble_status ble_st = ble_mgr_get_status();

	int64_t now = k_uptime_get();
	
	if (now - last_page_time >= 3000 || last_page_time == 0) {
		page_idx = (page_idx + 1) % 7;
		last_page_time = now;
	}

	anim_tick++;

	/* Scan for the next valid page starting from page_idx */
	for (int i = 0; i < 7; i++) {
		uint8_t current = (page_idx + i) % 7;

		cfb_framebuffer_clear(display, false);
		shown = false;

		if (current < 5) {
			k_mutex_lock(&config_mutex, K_FOREVER);
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
			k_mutex_unlock(&config_mutex);

			if (shown) {
				cfb_print(display, (char *)label, 0, 0);
				cfb_print(display, val_buf, 0, 20);
			}
		} else if (current == 5) {
			const char *status = sensors_get_status_word();
			cfb_print(display, ru ? "ВОЗД:" : "AIR:", 0, 0);
			cfb_print(display, (char *)status, 0, 20);
			shown = true;
		} else if (current == 6) {
			char ble_desc[32];
			int x_offset = 0;

			if (ble_st == BLE_STATUS_CONNECTED) {
				snprintf(ble_desc, sizeof(ble_desc), "%s", ru ? "СВЯЗЬ: OK" : "LINK: OK");
			} else if (ble_st == BLE_STATUS_CONNECTING) {
				/* Animated Connecting text */
				snprintf(ble_desc, sizeof(ble_desc), "%s%s", 
				         ru ? "ПОИСК" : "WAIT",
				         (anim_tick % 4 == 0) ? "" : (anim_tick % 4 == 1) ? "." : (anim_tick % 4 == 2) ? ".." : "...");
				
				/* Simple back-and-forth scroll by 0..20 pixels */
				int cycle = anim_tick % 20;
				x_offset = (cycle < 10) ? cycle * 2 : (20 - cycle) * 2;
			} else {
				snprintf(ble_desc, sizeof(ble_desc), "%s", ru ? "ОТКЛЮЧ" : "DISCONN");
			}

			cfb_print(display, "BLE:", 0, 0);
			cfb_print(display, ble_desc, x_offset, 20);
			shown = true;
		}

		if (shown) {
			cfb_framebuffer_finalize(display);
			/* Update page_idx so we don't skip this page later if still within the 3 seconds */
			page_idx = current;
			break;
		}
	}
}

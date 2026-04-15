#include "display_mgr.h"
#include "app_config.h"
#include "sensors.h"
#include <zephyr/display/cfb.h>
#include <stdio.h>

const struct device *display = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));

int display_mgr_init(void)
{
	if (!device_is_ready(display)) return -ENODEV;
	if (display_set_pixel_format(display, PIXEL_FORMAT_MONO10) < 0) return -EIO;
	if (cfb_framebuffer_init(display)) return -EIO;

	cfb_framebuffer_clear(display, true);
	cfb_framebuffer_set_font(display, 0);

	return 0;
}

void display_mgr_update(void)
{
	static uint8_t page_idx = 0;
	char val_buf[32];
	const char *label = "";
	bool shown = false;
	bool ru = config.lang_ru;

	k_mutex_lock(&config_mutex, K_FOREVER);

	/* Total pages: 5 metrics + 1 status screen = 6 */
	for (int i = 0; i < 6; i++) {
		uint8_t current = (page_idx + i) % 6;

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
					label = "t °C";
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
		} else {
			/* Status screen Page */
			const char *status = sensors_get_status_word();
			cfb_print(display, ru ? "ВОЗД:" : "AIR:", 0, 0);
			cfb_print(display, (char *)status, 0, 20);
			shown = true;
		}

		if (shown) {
			cfb_framebuffer_finalize(display);
			page_idx = (current + 1) % 6;
			break;
		}
	}

	k_mutex_unlock(&config_mutex);
}

#include "app_config.h"
#include <zephyr/settings/settings.h>
#include <string.h>
#include <stdio.h>

struct app_config config = {
	.interval_ms = 5000,
	.measure_co2 = true,
	.measure_temp = true,
	.measure_hum = true,
	.measure_dp = true,
	.measure_vpd = true,
	.active = true,
	.log_to_console = true,
	.lang_ru = true  /* Default to RU */
};

struct measurement_data last_data = {0};
K_MUTEX_DEFINE(config_mutex);

static int app_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	const char *next;
	int ret;

	if (settings_name_steq(name, "interval", &next) && !next) {
		ret = read_cb(cb_arg, &config.interval_ms, sizeof(config.interval_ms));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "m_co2", &next) && !next) {
		ret = read_cb(cb_arg, &config.measure_co2, sizeof(config.measure_co2));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "m_temp", &next) && !next) {
		ret = read_cb(cb_arg, &config.measure_temp, sizeof(config.measure_temp));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "m_hum", &next) && !next) {
		ret = read_cb(cb_arg, &config.measure_hum, sizeof(config.measure_hum));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "m_dp", &next) && !next) {
		ret = read_cb(cb_arg, &config.measure_dp, sizeof(config.measure_dp));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "m_vpd", &next) && !next) {
		ret = read_cb(cb_arg, &config.measure_vpd, sizeof(config.measure_vpd));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "active", &next) && !next) {
		ret = read_cb(cb_arg, &config.active, sizeof(config.active));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "log", &next) && !next) {
		ret = read_cb(cb_arg, &config.log_to_console, sizeof(config.log_to_console));
		return (ret < 0) ? ret : 0;
	}
	if (settings_name_steq(name, "lang_ru", &next) && !next) {
		ret = read_cb(cb_arg, &config.lang_ru, sizeof(config.lang_ru));
		return (ret < 0) ? ret : 0;
	}

	return 0;
}

struct settings_handler app_settings_conf = {
	.name = "app",
	.h_set = app_settings_set
};

int app_config_init(void)
{
	int err = settings_subsys_init();
	if (err) return err;

	err = settings_register(&app_settings_conf);
	if (err) return err;

	return settings_load();
}

int app_config_save(void)
{
	settings_save_one("app/interval", &config.interval_ms, sizeof(config.interval_ms));
	settings_save_one("app/m_co2", &config.measure_co2, sizeof(config.measure_co2));
	settings_save_one("app/m_temp", &config.measure_temp, sizeof(config.measure_temp));
	settings_save_one("app/m_hum", &config.measure_hum, sizeof(config.measure_hum));
	settings_save_one("app/m_dp", &config.measure_dp, sizeof(config.measure_dp));
	settings_save_one("app/m_vpd", &config.measure_vpd, sizeof(config.measure_vpd));
	settings_save_one("app/active", &config.active, sizeof(config.active));
	settings_save_one("app/log", &config.log_to_console, sizeof(config.log_to_console));
	settings_save_one("app/lang_ru", &config.lang_ru, sizeof(config.lang_ru));
	return 0;
}

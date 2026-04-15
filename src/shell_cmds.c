#include "app_config.h"
#include "sensors.h"
#include "display_mgr.h"
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor/scd4x.h>
#include <stdlib.h>
#include <string.h>

static int cmd_status(const struct shell *shell, size_t argc, char **argv)
{
	bool ru = config.lang_ru;
	k_mutex_lock(&config_mutex, K_FOREVER);
	shell_print(shell, ru ? "Система: %s" : "System: %s", config.active ? (ru ? "Активна" : "Active") : (ru ? "Пауза" : "Paused"));
	shell_print(shell, ru ? "Статус: %s" : "Status: %s", sensors_get_status_word());
	shell_print(shell, ru ? "Интервал: %d мс" : "Interval: %d ms", config.interval_ms);
	if (config.timer_min > 0) {
		shell_print(shell, ru ? "Таймер: %d мин осталось" : "Timer: %d min remaining", config.timer_min);
	}
	shell_print(shell, ru ? "Язык: Русский" : "Language: English");
	k_mutex_unlock(&config_mutex);
	return 0;
}

static int cmd_interval(const struct shell *shell, size_t argc, char **argv)
{
	uint32_t ms = atoi(argv[1]);
	if (ms < 1000) {
		shell_error(shell, "Interval too short (min 1000ms)");
		return -EINVAL;
	}
	k_mutex_lock(&config_mutex, K_FOREVER);
	config.interval_ms = ms;
	k_mutex_unlock(&config_mutex);
	app_config_save();
	shell_print(shell, "Interval set to %d ms", ms);
	return 0;
}

static int cmd_enable(const struct shell *shell, size_t argc, char **argv)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	if (strcmp(argv[1], "co2") == 0) config.measure_co2 = true;
	else if (strcmp(argv[1], "temp") == 0) config.measure_temp = true;
	else if (strcmp(argv[1], "hum") == 0) config.measure_hum = true;
	else if (strcmp(argv[1], "dp") == 0) config.measure_dp = true;
	else if (strcmp(argv[1], "vpd") == 0) config.measure_vpd = true;
	else if (strcmp(argv[1], "all") == 0) {
		config.measure_co2 = config.measure_temp = config.measure_hum = 
		config.measure_dp = config.measure_vpd = true;
	} else {
		shell_error(shell, "Unknown parameter: %s", argv[1]);
	}
	k_mutex_unlock(&config_mutex);
	app_config_save();
	return 0;
}

static int cmd_disable(const struct shell *shell, size_t argc, char **argv)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	if (strcmp(argv[1], "co2") == 0) config.measure_co2 = false;
	else if (strcmp(argv[1], "temp") == 0) config.measure_temp = false;
	else if (strcmp(argv[1], "hum") == 0) config.measure_hum = false;
	else if (strcmp(argv[1], "dp") == 0) config.measure_dp = false;
	else if (strcmp(argv[1], "vpd") == 0) config.measure_vpd = false;
	else if (strcmp(argv[1], "all") == 0) {
		config.measure_co2 = config.measure_temp = config.measure_hum = 
		config.measure_dp = config.measure_vpd = false;
	}
	k_mutex_unlock(&config_mutex);
	app_config_save();
	return 0;
}

static int cmd_start_stop(const struct shell *shell, size_t argc, char **argv)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	if (strcmp(argv[0], "start") == 0) config.active = true;
	else config.active = false;
	k_mutex_unlock(&config_mutex);
	app_config_save();
	return 0;
}

static int cmd_lang(const struct shell *shell, size_t argc, char **argv)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	if (strcmp(argv[1], "ru") == 0) {
		config.lang_ru = true;
		shell_print(shell, "Язык изменен на Русский");
	} else if (strcmp(argv[1], "en") == 0) {
		config.lang_ru = false;
		shell_print(shell, "Language changed to English");
	} else {
		shell_error(shell, "Usage: scd lang <ru|en>");
	}
	k_mutex_unlock(&config_mutex);
	app_config_save();
	return 0;
}

static int cmd_asc(const struct shell *shell, size_t argc, char **argv)
{
	struct sensor_value attr;
	if (strcmp(argv[1], "on") == 0) attr.val1 = 1;
	else attr.val1 = 0;

	int ret = sensor_attr_set(sensor, SENSOR_CHAN_ALL,
				 (enum sensor_attribute)SENSOR_ATTR_SCD4X_AUTOMATIC_CALIB_ENABLE,
				 &attr);
	if (ret) {
		shell_error(shell, "Failed to set ASC: %d", ret);
		return ret;
	}
	shell_print(shell, "ASC set to %s", attr.val1 ? "ON" : "OFF");
	return 0;
}

static int cmd_info(const struct shell *shell, size_t argc, char **argv)
{
	sensors_print_formulas(shell);
	return 0;
}

static int cmd_timer(const struct shell *shell, size_t argc, char **argv)
{
	int32_t min = atoi(argv[1]);
	k_mutex_lock(&config_mutex, K_FOREVER);
	config.timer_min = min;
	k_mutex_unlock(&config_mutex);
	shell_print(shell, "Timer set to %d minutes", min);
	return 0;
}

static int cmd_advice(const struct shell *shell, size_t argc, char **argv)
{
	sensors_give_advice(shell);
	return 0;
}

static int cmd_log_on(const struct shell *shell, size_t argc, char **argv)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	config.log_to_console = true;
	k_mutex_unlock(&config_mutex);
	app_config_save();
	shell_print(shell, "Logging: ON");
	return 0;
}

static int cmd_log_off(const struct shell *shell, size_t argc, char **argv)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	config.log_to_console = false;
	k_mutex_unlock(&config_mutex);
	app_config_save();
	shell_print(shell, "Logging: OFF");
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_scd,
	SHELL_CMD_ARG(status, NULL, "Show current settings", cmd_status, 1, 0),
	SHELL_CMD_ARG(interval, NULL, "Set measurement interval in ms", cmd_interval, 2, 0),
	SHELL_CMD_ARG(enable, NULL, "Enable metric {co2|temp|hum|dp|vpd|all}", cmd_enable, 2, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable metric {co2|temp|hum|dp|vpd|all}", cmd_disable, 2, 0),
	SHELL_CMD_ARG(asc, NULL, "Auto-calibration {on|off}", cmd_asc, 2, 0),
	SHELL_CMD_ARG(info, NULL, "Show detailed metric interpretations", cmd_info, 1, 0),
	SHELL_CMD_ARG(advice, NULL, "Get smart advice for current air quality", cmd_advice, 1, 0),
	SHELL_CMD_ARG(timer, NULL, "Set auto-stop timer in minutes (0 to disable)", cmd_timer, 2, 0),
	SHELL_CMD_ARG(lang, NULL, "Change language {ru|en}", cmd_lang, 2, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(scd, &sub_scd, "SCD4X sensor commands", NULL);
SHELL_CMD_REGISTER(start, NULL, "Start measurements", cmd_start_stop);
SHELL_CMD_REGISTER(stop, NULL, "Stop measurements", cmd_start_stop);
SHELL_CMD_REGISTER(l1, NULL, "Enable console logging", cmd_log_on);
SHELL_CMD_REGISTER(l0, NULL, "Disable console logging", cmd_log_off);

void shell_cmds_init(void) {}

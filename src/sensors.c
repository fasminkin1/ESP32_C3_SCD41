#include "sensors.h"
#include "app_config.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/scd4x.h>
#include <zephyr/shell/shell.h>
#include <math.h>
#include <stdio.h>

const struct device *sensor = DEVICE_DT_GET(DT_NODELABEL(scd41));

static double calculate_dewpoint(double temp, double hum)
{
	if (hum < 0.1) return 0.0; /* Safety check for log(0) */
	const double a = 17.27;
	const double b = 237.7;
	double alpha = ((a * temp) / (b + temp)) + log(hum / 100.0);
	return (b * alpha) / (a - alpha);
}

static double calculate_vpd(double temp, double hum)
{
	if (temp < -40.0) return 0.0;
	double es = 0.61078 * exp(17.27 * temp / (temp + 237.3));
	return es * (1.0 - (hum / 100.0));
}

void sensors_print_formulas(const struct shell *shell)
{
	bool ru = config.lang_ru;
	if (ru) {
		shell_print(shell, "\n=== ИНТЕРПРЕТАЦИЯ ДАННЫХ ===");
		shell_print(shell, "1. CO2 (PPM): Свежесть воздуха");
		shell_print(shell, "   400-800: ОТЛИЧНО. Высокая продуктивность.");
		shell_print(shell, "   800-1000: ХОРОШО. Норма для помещения.");
		shell_print(shell, "   1000-1500: ПЛОХО. Вялость и сонливость.");
		shell_print(shell, "   >1500: ОПАСНО. Головная боль, мозг 'отключается'.");
		shell_print(shell, "\n2. ВЛАЖНОСТЬ (%%): Здоровье слизистых");
		shell_print(shell, "   30-60%%: ОПТИМАЛЬНО. Идеально для глаз и легких.");
		shell_print(shell, "   <30%%: СУХО. Риск вирусов и сухой кожи.");
		shell_print(shell, "\n3. VPD (kPa): Баланс испарения");
		shell_print(shell, "   0.8-1.2: ИДЕАЛЬНО.");
		shell_print(shell, "============================\n");
	} else {
		shell_print(shell, "\n=== DATA INTERPRETATION ===");
		shell_print(shell, "1. CO2 (PPM): Air freshness");
		shell_print(shell, "   400-800: EXCELLENT. High productivity.");
		shell_print(shell, "   800-1000: GOOD. Normal indoor level.");
		shell_print(shell, "   1000-1500: POOR. Fatigue and sleepiness.");
		shell_print(shell, "   >1500: BAD. Headache, 'brain off' syndrome.");
		shell_print(shell, "\n2. HUMIDITY (%%): Comfort/Health");
		shell_print(shell, "   30-60%%: OPTIMAL. Best for mucosa and skin.");
		shell_print(shell, "   <30%%: DRY. Risk of viruses.");
		shell_print(shell, "\n3. VPD (kPa): Evaporation balance");
		shell_print(shell, "   0.8-1.2: IDEAL.");
		shell_print(shell, "============================\n");
	}
}

enum app_status sensors_get_status_code(void)
{
	if (last_data.co2 > RANGE_CO2_MAX) return STATUS_CRIT;
	if (last_data.co2 > 1000) return STATUS_WARN;
	if (last_data.hum < RANGE_HUM_MIN || last_data.hum > RANGE_HUM_MAX) return STATUS_WARN;
	return STATUS_OK;
}

const char* sensors_get_status_word(void)
{
	bool ru = config.lang_ru;
	enum app_status code = sensors_get_status_code();
	
	if (code == STATUS_CRIT) return ru ? "BAD!" : "BAD!";
	if (code == STATUS_WARN) return ru ? "WARN" : "WARN";
	return ru ? "GOOD" : "GOOD";
}

void sensors_give_advice(const struct shell *shell)
{
	shell_print(shell, "%s", sensors_get_advice_string());
}

const char* sensors_get_advice_string(void)
{
	bool ru = config.lang_ru;

	if (last_data.co2 > 1500) {
		return ru ? "!!! КРИТИЧЕСКИЙ CO2: ПРОВЕТРИТЕ!" : "!!! CRITICAL CO2: VENTILATE!";
	} else if (last_data.co2 > 1000) {
		return ru ? "! ПЛОХОЙ ВОЗДУХ: Проветрите." : "! POOR CO2: Ventilate.";
	} else if (last_data.hum < 30.0) {
		return ru ? "! СУХО: Нужен увлажнитель." : "! DRY AIR: Need humidifier.";
	} else if (last_data.hum > 70.0) {
		return ru ? "! ВЛАЖНО: Плесень/грибок!" : "! HUMID: Risk of mold!";
	}

	return ru ? "Все в норме. Хорошего дня!" : "Everything OK. Have a nice day!";
}

int sensors_init(void)
{
	if (!device_is_ready(sensor)) return -ENODEV;
	struct sensor_value attr = { .val1 = 1 };
	(void)sensor_attr_set(sensor, SENSOR_CHAN_ALL,
			    (enum sensor_attribute)SENSOR_ATTR_SCD4X_AUTOMATIC_CALIB_ENABLE,
			    &attr);
	return 0;
}

int sensors_fetch_all(void)
{
	struct sensor_value co2_sv, temp_sv, hum_sv;
	int ret = sensor_sample_fetch(sensor);
	if (ret) return ret;

	sensor_channel_get(sensor, SENSOR_CHAN_CO2, &co2_sv);
	sensor_channel_get(sensor, SENSOR_CHAN_AMBIENT_TEMP, &temp_sv);
	sensor_channel_get(sensor, SENSOR_CHAN_HUMIDITY, &hum_sv);

	k_mutex_lock(&config_mutex, K_FOREVER);
	last_data.co2 = co2_sv.val1;
	last_data.temp = sensor_value_to_double(&temp_sv);
	last_data.hum = sensor_value_to_double(&hum_sv);

	/* Only calculate if sensors returned valid data */
	if (last_data.hum > 0.1) {
		last_data.dp = calculate_dewpoint(last_data.temp, last_data.hum);
		last_data.vpd = calculate_vpd(last_data.temp, last_data.hum);
	} else {
		last_data.dp = 0.0;
		last_data.vpd = 0.0;
	}

	if (config.log_to_console) {
		const char* status = sensors_get_status_word();
		printf("[%s] CO2=%d, T=%.2f, H=%.1f\n",
			status, last_data.co2, last_data.temp, last_data.hum);
	}
	k_mutex_unlock(&config_mutex);
	return 0;
}

int sensors_build_telemetry_json(char *buf, size_t len)
{
	k_mutex_lock(&config_mutex, K_FOREVER);
	
	/* Use short keys to save space: 
	   val: values (c=co2, t=temp, h=hum, d=dp, v=vpd)
	   cfg: config (i=interval, a=active, t=timer)
	   s: status code
	*/
	int ret = snprintf(buf, len,
		"{\"val\":{\"c\":%d,\"t\":%.1f,\"h\":%.1f,\"d\":%.1f,\"v\":%.2f},"
		"\"cfg\":{\"i\":%d,\"a\":%d,\"tm\":%d},"
		"\"s\":%d}",
		last_data.co2, last_data.temp, last_data.hum, 
		last_data.dp, last_data.vpd,
		config.interval_ms, config.active ? 1 : 0, config.timer_min,
		sensors_get_status_code());
		
	k_mutex_unlock(&config_mutex);
	
	return (ret > 0 && ret < len) ? 0 : -ENOMEM;
}

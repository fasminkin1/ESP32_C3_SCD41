#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <zephyr/kernel.h>
#include <stdbool.h>

#define APP_VERSION "v1.0.1"

/* Comfort Ranges */
#define RANGE_CO2_MAX    1500
#define RANGE_CO2_MIN    350
#define RANGE_TEMP_MAX   35.0
#define RANGE_TEMP_MIN   15.0
#define RANGE_HUM_MAX    75.0
#define RANGE_HUM_MIN    25.0
#define RANGE_VPD_MAX    2.0
#define RANGE_VPD_MIN    0.4

/* Global configuration state */
struct app_config {
	uint32_t interval_ms;
	bool measure_co2;
	bool measure_temp;
	bool measure_hum;
	bool measure_dp;
	bool measure_vpd;
	bool active;
	bool log_to_console;
	bool lang_ru;  /* Language toggle: false = EN, true = RU */
	int32_t timer_min; /* Minutes until auto-stop. 0 = disabled */
};

/* Simplified measurement data for logic and display */
struct measurement_data {
	int co2;
	double temp;
	double hum;
	double dp;
	double vpd;
};

extern struct app_config config;
extern struct measurement_data last_data;
extern struct k_mutex config_mutex;

enum app_status {
	STATUS_OK = 0,
	STATUS_WARN = 1,
	STATUS_CRIT = 2
};

int app_config_init(void);
int app_config_save(void);

/* Helper to get numeric status */
enum app_status sensors_get_status_code(void);

/* Helper to build JSON telemetry string */
int sensors_build_telemetry_json(char *buf, size_t len);

#endif

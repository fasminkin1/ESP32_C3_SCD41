#ifndef SENSORS_H
#define SENSORS_H

#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

extern const struct device *sensor;

int sensors_init(void);
int sensors_fetch_all(void);
void sensors_print_formulas(const struct shell *shell);
void sensors_give_advice(const struct shell *shell);
const char* sensors_get_status_word(void);
const char* sensors_get_advice_string(void);

#endif

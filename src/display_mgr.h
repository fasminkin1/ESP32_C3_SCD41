#ifndef DISPLAY_MGR_H
#define DISPLAY_MGR_H

#include <zephyr/device.h>

extern const struct device *display;

int display_mgr_init(void);
void display_mgr_show_splash(void);
void display_mgr_update(void);

#endif

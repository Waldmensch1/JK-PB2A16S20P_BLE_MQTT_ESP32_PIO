#ifndef SETTINGS_H
#define SETTINGS_H
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

extern uint16_t publish_delay;

void init_settings();
void write_setting(const char *setting_name, uint16_t value);
uint16_t read_setting(const char *setting_name, uint16_t default_value);
void re_read_settings();

#endif // SETTINGS_H
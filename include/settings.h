#ifndef SETTINGS_H
#define SETTINGS_H
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>
#include <nvs_flash.h>

extern uint16_t publish_delay;
extern bool debug_flg;
extern bool debug_flg_full_log;

void init_settings();
void write_setting(const char *setting_name, uint16_t value);
void write_setting(const char *setting_name, bool value);
uint16_t read_setting(const char *setting_name, uint16_t default_value);
bool read_setting(const char *setting_name, bool default_value);
void re_read_settings();

#endif // SETTINGS_H
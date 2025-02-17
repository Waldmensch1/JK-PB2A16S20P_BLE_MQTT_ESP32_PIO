#ifndef LED_CONTROL_H
#define LED_CONTROL_H
#include <Arduino.h>

#ifdef USELED
#define LED_PIN USELED

enum LedState {
    LED_OFF,
    LED_ON,
    LED_BLINK_SLOW,
    LED_BLINK_FAST,
    LED_DOUBLE_FLASH,
    LED_FLASH
};

void init_led();
void set_led(LedState state);
#endif //USELED

#endif // LED_CONTROL_H

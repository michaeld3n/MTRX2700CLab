#ifndef LED_H
#define LED_H

#include <stdint.h>

#define LED_COUNT      8
#define LED_PIN_OFFSET 8  // LEDs are on GPIOE pins 8-15

// Opaque pointer - struct is hidden in led.c
typedef struct LED_Module LED_Module;

LED_Module* led_init    (void);
int         led_set     (LED_Module *module, uint8_t index, uint8_t state);
int         led_get     (LED_Module *module, uint8_t index);
void        led_clear_all(LED_Module *module);

#endif

#include "led.h"
#include "gpio.h"
#include "stm32f303xc.h"
#include <stdlib.h>

// Struct defined here so outside files cannot access states directly
struct LED_Module {
    uint8_t states[LED_COUNT];
};

// Enable GPIOE clock, set pins 8-15 to output, start all LEDs off
LED_Module* led_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOEEN;

    LED_Module *module = (LED_Module*)malloc(sizeof(LED_Module));
    if (module == NULL) return NULL;

    // 0x5555 sets all 8 pins to output mode at once
    uint16_t *led_output_registers = ((uint16_t*)&(GPIOE->MODER)) + 1;
    *led_output_registers = 0x5555;

    // Clear upper byte of ODR to turn all LEDs off
    uint8_t *led_register = ((uint8_t*)&(GPIOE->ODR)) + 1;
    *led_register = 0x00;

    int i;
    for (i = 0; i < LED_COUNT; i++) module->states[i] = 0;

    return module;
}

// Update internal state then write to the physical pin via gpio_write
int led_set(LED_Module *module, uint8_t index, uint8_t state)
{
    if (module == NULL || index >= LED_COUNT) return 0;

    state = (state != 0) ? 1 : 0;
    module->states[index] = state;

    gpio_write(GPIOE, index + LED_PIN_OFFSET,
               state ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
    return 1;
}

// Read from internal states array not directly from hardware
int led_get(LED_Module *module, uint8_t index)
{
    if (module == NULL || index >= LED_COUNT) return -1;
    return module->states[index];
}

// Clear internal states and zero the ODR upper byte
void led_clear_all(LED_Module *module)
{
    if (module == NULL) return;

    int i;
    for (i = 0; i < LED_COUNT; i++) module->states[i] = 0;

    uint8_t *led_register = ((uint8_t*)&(GPIOE->ODR)) + 1;
    *led_register = 0x00;
}

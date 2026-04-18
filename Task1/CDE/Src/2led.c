#include "2led.h"
#include "2gpio.h"
#include "stm32f303xc.h"
#include <stdlib.h>

struct LED_Module {
    uint8_t states[LED_COUNT];
};

LED_Module* led_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOEEN;

    LED_Module *module = (LED_Module*)malloc(sizeof(LED_Module));
    if (module == NULL) return NULL;

    uint16_t *led_output_registers = ((uint16_t*)&(GPIOE->MODER)) + 1;
    *led_output_registers = 0x5555;

    uint8_t *led_register = ((uint8_t*)&(GPIOE->ODR)) + 1;
    *led_register = 0x00;

    int i;
    for (i = 0; i < LED_COUNT; i++) module->states[i] = 0;

    return module;
}

// Applies changes immediately - no queuing
int led_set(LED_Module *module, uint8_t index, uint8_t state)
{
    if (module == NULL || index >= LED_COUNT) return 0;

    state = (state != 0) ? 1 : 0;
    module->states[index] = state;

    gpio_write(GPIOE, index + LED_PIN_OFFSET,
               state ? GPIO_STATE_HIGH : GPIO_STATE_LOW);
    return 1;
}

int led_get(LED_Module *module, uint8_t index)
{
    if (module == NULL || index >= LED_COUNT) return -1;
    return module->states[index];
}

void led_clear_all(LED_Module *module)
{
    if (module == NULL) return;

    int i;
    for (i = 0; i < LED_COUNT; i++) module->states[i] = 0;

    uint8_t *led_register = ((uint8_t*)&(GPIOE->ODR)) + 1;
    *led_register = 0x00;
}

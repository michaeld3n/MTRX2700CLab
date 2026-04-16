#include "stm32f303xc.h"
#include "2gpio.h"
#include "2led.h"
#include "2button.h"

static LED_Module    *leds   = NULL;
static Button_Module *button = NULL;
static int current_mode = 0;

void on_button_press(void)
{
    if (current_mode == 0) {
        led_set(leds, 0, 0); // North off
        led_set(leds, 4, 0); // South off
        led_set(leds, 2, 1); // East on
        led_set(leds, 6, 1); // West on
        current_mode = 1;
    } else {
        led_set(leds, 2, 0); // East off
        led_set(leds, 6, 0); // West off
        led_set(leds, 0, 1); // North on
        led_set(leds, 4, 1); // South on
        current_mode = 0;
    }
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & (1 << 0)) {
        EXTI->PR |= (1 << 0);
        button_interrupt_handler(button);
    }
}

int main(void)
{
    RCC->AHBENR  |= RCC_AHBENR_GPIOAEN |
                    RCC_AHBENR_GPIOCEN  |
                    RCC_AHBENR_GPIOEEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    volatile int delay = 10000;
    while (delay--);

    leds   = led_init();
    button = button_init(on_button_press);

    // --------------------------------------------------------
    // TASK C - Interrupt driven button
    // --------------------------------------------------------
    led_set(leds, 0, 1); // North on
    led_set(leds, 4, 1); // South on

    // --------------------------------------------------------
    // TASK D - only way to change LEDs is through led_set
    // --------------------------------------------------------
    while (1) {
    }
}

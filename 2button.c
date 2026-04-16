#include "2button.h"
#include "2gpio.h"
#include <stdlib.h>

struct Button_Module {
    ButtonCallback on_press;
};

Button_Module* button_init(ButtonCallback callback)
{
    // Enable GPIOA clock
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    Button_Module *module = (Button_Module*)malloc(sizeof(Button_Module));
    if (module == NULL) return NULL;

    module->on_press = callback;

    // Set PA0 as input
    gpio_init(BUTTON_PORT, BUTTON_PIN, GPIO_DIR_INPUT);

    // Enable SYSCFG clock - needed before we can configure EXTI
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Small delay to let the clock stabilise before configuring EXTI
    volatile int delay = 100;
    while (delay--);

    // Route PA0 to EXTI line 0
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;
    SYSCFG->EXTICR[0] |=  SYSCFG_EXTICR1_EXTI0_PA;

    // Unmask EXTI line 0
    EXTI->IMR |= (1 << BUTTON_PIN);

    // Set rising edge trigger, clear falling edge
    EXTI->RTSR |=  (1 << BUTTON_PIN);
    EXTI->FTSR &= ~(1 << BUTTON_PIN);

    // Clear any pending interrupt before enabling
    EXTI->PR |= (1 << BUTTON_PIN);

    // Enable in NVIC
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);

    return module;
}

int button_is_pressed(Button_Module *module)
{
    if (module == NULL) return 0;

    if (gpio_read(BUTTON_PORT, BUTTON_PIN) == GPIO_STATE_HIGH) {
        return 1;
    }
    return 0;
}

void button_interrupt_handler(Button_Module *module)
{
    if (module == NULL) return;

    if (module->on_press != NULL) {
        module->on_press();
    }
}

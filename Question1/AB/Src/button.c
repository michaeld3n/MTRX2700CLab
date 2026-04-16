#include "button.h"
#include "gpio.h"
#include <stdlib.h>

struct Button_Module {
    uint8_t placeholder;
};

// Enable GPIOA clock and set PA0 as input
Button_Module* button_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;

    Button_Module *module = (Button_Module*)malloc(sizeof(Button_Module));
    if (module == NULL) return NULL;

    gpio_init(BUTTON_PORT, BUTTON_PIN, GPIO_DIR_INPUT);
    return module;
}

// Read PA0 state, returns 1 if held down, 0 if not
int button_is_pressed(Button_Module *module)
{
    if (module == NULL) return 0;

    if (gpio_read(BUTTON_PORT, BUTTON_PIN) == GPIO_STATE_HIGH) {
        return 1;
    }
    return 0;
}

#include "gpio.h"

// Set a pin as input(00) or output(01) via the MODER register
int gpio_init(GPIO_TypeDef *port, uint8_t pin, GPIO_Direction dir)
{
    if (port == NULL || pin > 15) return 0;

    port->MODER &= ~(0x3 << (pin * 2)); // clear the 2 bits for this pin
    if (dir == GPIO_DIR_OUTPUT) {
        port->MODER |= (0x1 << (pin * 2)); // set to 01 = output
    }
    return 1;
}

// Set pin high or low using BSRR
int gpio_write(GPIO_TypeDef *port, uint8_t pin, GPIO_State state)
{
    if (port == NULL || pin > 15) return 0;

    if (state == GPIO_STATE_HIGH) {
        port->BSRR = (1 << pin);
    } else {
        port->BSRR = (1 << (pin + 16));
    }
    return 1;
}

// Read live pin state from IDR register
GPIO_State gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    if (port == NULL || pin > 15) return GPIO_STATE_LOW;

    if (port->IDR & (1 << pin)) {
        return GPIO_STATE_HIGH;
    }
    return GPIO_STATE_LOW;
}

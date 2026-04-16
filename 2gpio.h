#ifndef GPIO_H
#define GPIO_H

#include "stm32f303xc.h"
#include <stdint.h>
#include <stddef.h>

typedef enum { GPIO_DIR_INPUT = 0, GPIO_DIR_OUTPUT = 1 } GPIO_Direction;
typedef enum { GPIO_STATE_LOW = 0, GPIO_STATE_HIGH = 1 } GPIO_State;

// Setup, write and read any pin on any port
int        gpio_init (GPIO_TypeDef *port, uint8_t pin, GPIO_Direction dir);
int        gpio_write(GPIO_TypeDef *port, uint8_t pin, GPIO_State state);
GPIO_State gpio_read (GPIO_TypeDef *port, uint8_t pin);

#endif

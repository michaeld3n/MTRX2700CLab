#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f303xc.h"

#define BUTTON_PIN  0
#define BUTTON_PORT GPIOA

// Function pointer type for the callback
typedef void (*ButtonCallback)(void);

// Opaque pointer - struct contents hidden in 2button.c
typedef struct Button_Module Button_Module;

Button_Module* button_init             (ButtonCallback callback);
int            button_is_pressed       (Button_Module *module);
void           button_interrupt_handler(Button_Module *module);

#endif

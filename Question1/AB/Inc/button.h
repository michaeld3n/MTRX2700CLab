#ifndef BUTTON_H
#define BUTTON_H

#include "stm32f303xc.h"

#define BUTTON_PIN  0
#define BUTTON_PORT GPIOA

// Opaque pointer - struct contents hidden in button.c
typedef struct Button_Module Button_Module;

Button_Module* button_init      (void);
int            button_is_pressed(Button_Module *module);

#endif

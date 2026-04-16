#include "stm32f303xc.h"
#include "gpio.h"
#include "led.h"
#include "button.h"

int main(void)
{
    // Enable clocks for GPIOA, GPIOC and GPIOE
    // The STM32 disables clocks by default to save power
    // Nothing will work on these ports until we turn them on
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN |
                   RCC_AHBENR_GPIOCEN  |
                   RCC_AHBENR_GPIOEEN;


    // --------------------------------------------------------
    // TASK A - Generic GPIO
    // Here we use the gpio functions directly on a single pin
    // without any module on top to show the foundation layer works
    // Setting PE8 high should light up the first LED on the board
    // --------------------------------------------------------
    gpio_init(GPIOE, 8, GPIO_DIR_OUTPUT);
    gpio_write(GPIOE, 8, GPIO_STATE_HIGH);


    // --------------------------------------------------------
    // TASK B - LED and Button Modules
    // Both modules are built on top of the Task A gpio functions
    // led_init sets up all 8 LEDs on GPIOE pins 8-15 as outputs
    // button_init sets up the blue user button on PA0 as an input
    // Turning on LEDs 0, 3 and 7 confirms the module is working
    // --------------------------------------------------------
    LED_Module    *leds   = led_init();
    Button_Module *button = button_init();

    // Start with North and South on
       led_set(leds, 1, 1); // North on
       led_set(leds, 5, 1); // South on


    // --------------------------------------------------------
    // TASK B - Polling Loop
    // Polling means we constantly check the button state in a loop
    // The CPU does nothing else - it just keeps reading the pin
    // While the button is held LED 4 turns on
    // When the button is released LED 4 turns off
    // In Task C we will replace this with an interrupt approach
    // where the CPU does not need to keep checking
    // --------------------------------------------------------
    while (1) {
            if (button_is_pressed(button)) {
                // button held - show East and West
                led_set(leds, 1, 0); // North off
                led_set(leds, 5, 0); // South off
                led_set(leds, 3, 1); // East on
                led_set(leds, 7, 1); // West on
            } else {
                // button not pressed - show North and South
                led_set(leds, 3, 0); // East off
                led_set(leds, 7, 0); // West off
                led_set(leds, 1, 1); // North on
                led_set(leds, 5, 1); // South on
            }
        }
    }

# MTRX2700 Lab 2 – Project Brief
## Programming a Microcontroller in C

---

## Exercise 1 – Digital I/O

### Step-by-step Tasks

- **a)** Create a generic GPIO module that can configure any GPIO pin as input or output, and provides access functions to read from or write to a specific pin.
- **b)** Building on the generic GPIO module, create separate independent modules that encapsulate interfaces to the Discovery Board's specific digital I/O: the user button and the LED array.
- **c)** Allow a function pointer to be passed into the module on initialisation. This function pointer is called as a callback whenever a button press is detected.
- **d)** Incorporate the LED state inside the module. The LED state must not be accessible directly from outside — all access must go through get/set functions defined in the header file.
- **e) (Advanced)** Restrict the rate at which the LEDs can change state using a hardware timer. The set LED function must return immediately (no polling delay inside it) — the rate-limiting must happen asynchronously.

### Expected End Results

- A GPIO module with clean initialisation, read, and write functions.
- A button module that fires a user-supplied callback on press without blocking.
- An LED module with fully encapsulated state, accessible only via get/set functions.
- (Advanced) LED changes are rate-limited by a timer interrupt, not a blocking delay.

---

## Exercise 2 – Timer Interface

### Step-by-step Tasks

- **a)** Implement a timer software module that triggers a callback function (via function pointer) at a regular interval. The interval must be passed in as a parameter during initialisation.
- **b)** Implement a function to reset the timer with a new period. The period value must not be accessible outside the module — expose it only through get/set functions.
- **c)** Use the timer module and the GPIO module from Exercise 1 to generate a PWM signal for a hobby servo:
  - Timer triggers at 50 Hz (20 ms period).
  - On trigger, drive the output pin HIGH.
  - Pull the pin LOW after a delay corresponding to the desired servo position:
    - 1 ms pulse → full clockwise
    - 1.5 ms pulse → centre
    - 2 ms pulse → full counterclockwise
- **d) (Advanced)** Implement a one-shot timer function: the timer fires once, triggers the callback, then stops. The function takes a delay in milliseconds and a callback function pointer as inputs.

### Expected End Results

- A timer module that calls a user callback at a configurable regular interval.
- Period is encapsulated; changed only through get/set functions.
- A working PWM output that can position a hobby servo based on a pulse width between 1–2 ms at 50 Hz.
- (Advanced) A one-shot delay function that fires exactly once then disarms.

---

## Exercise 3 – Serial Interface

### Step-by-step Tasks

- **a)** Create a UART module that can send an array of bytes from memory and receive an array of bytes given a known message structure. Base the design on the W06-UART-modular-design project.
- **b)** Implement a `sendString` function that accepts a pointer to a string and transmits it over USART1 (for PC console debugging).
- **c)** Implement a `sendMsg` function that:
  - Accepts a pointer to a memory block (a structure) and its size (use `sizeof`).
  - Assembles a data packet containing: start byte, message size, message type, message body, stop byte, and checksum.
  - Transmits the packet over a given serial port.
  - Supports sending different message types with different data structures.
- **d)** Implement a `receiveMsg` function that:
  - Reads incoming bytes into a memory buffer until a terminating character is received.
  - Verifies the checksum.
  - Calls a callback (registered at initialisation) when reception is complete, passing a pointer to the received memory block and the number of bytes received.
- **e)** Replace the polling-based receive implementation with an interrupt-driven approach. Data reception is considered complete when a terminating character is received via interrupt.
- **f) (Advanced)** Use interrupts for transmission as well as reception. Implement a double buffer so the serial module can continue receiving new data into one buffer while another part of the code consumes the previously received data from the other buffer.

### Expected End Results

- A UART module supporting both send and receive operations.
- `sendString` enables PC-side debug output over USART1.
- `sendMsg` produces correctly structured packets with checksum.
- `receiveMsg` validates and delivers received packets via callback.
- Receive (and advanced: transmit) is fully interrupt-driven.
- (Advanced) Double-buffered receive so data is never lost while being processed.

---

## Exercise 4 – I2C Sensor Interfacing

### Step-by-step Tasks

- **a)** Implement an I2C interface to the STM32F3 Discovery Board's built-in compass (magnetometer) module.
- **b)** Define a structure to store the magnetometer output. The structure must include:
  - Raw sensor data (X, Y, Z field values).
  - Decoded compass heading.
  - A timestamp indicating when the data was received.
  - This structure should be shareable with other modules or transmittable over UART.
- **c) (Advanced)** Extend the module to also read from the on-board accelerometer (same I2C device) and the MEMS gyroscope (available via SPI). Use these inputs to build a full attitude reference estimator that outputs heading, roll, and pitch.

### Expected End Results

- A working I2C driver that reads compass data from the magnetometer.
- A well-defined data structure containing raw values, decoded heading, and a timestamp.
- (Advanced) Full attitude estimation (heading, roll, pitch) using magnetometer, accelerometer, and gyroscope.

---

## Exercise 5 – Integration Task

### Step-by-step Tasks

- **a)** On **STM32 Board 1**: using all previously developed modules, read magnetometer data. Package the heading data into an appropriate structure and transmit it over UART to STM32 Board 2.
- **b)** On **STM32 Board 1**: implement an interrupt-driven button handler. When the button is pressed, set a signal (either as a field in the magnetometer structure or as a separate message type) that instructs Board 2 to change its display mode.
- **c)** On **STM32 Board 2**:
  - Wait to receive messages from Board 1 over UART.
  - In default mode: move the hobby servo to a position scaled from the received heading (0–360° mapped to the servo's ~90° range).
  - When the button-press signal is received: switch to LED display mode — light up the single LED from the 8-LED array that corresponds most closely to the current heading (e.g. 8 LEDs → 8 compass octants).

### Expected End Results

- Board 1 continuously reads compass heading and transmits structured UART packets to Board 2.
- Board 1 detects button presses via interrupt and includes the button state in its transmitted messages.
- Board 2 correctly receives and parses packets, drives the servo to the correct position, and switches to LED heading display when the button is pressed.
- All modules from Exercises 1–4 are reused without duplication; any fixes are reflected back in the original module projects.

---

## Project Overview

This project implements five C-language software modules for the STM32F3 Discovery Board: digital I/O (GPIO, button, LEDs), hardware timers/PWM (servo control), UART serial communication (interrupt-driven, structured packets), I2C sensor interfacing (magnetometer/compass), and a final integration task connecting two boards via UART to drive a servo and LED array from live compass heading data.

Group members: Michael Deng (Exercise 4/5), William Kirk (Exercise 1), Josh Johnson (Exercise 2), Hajin Paek (Exercise 3).

### Summary

Five modular C drivers that each encapsulate one hardware subsystem. Each module exposes a clean header-file interface and hides implementation details in the `.c` file. Modules are composable — the integration task (Exercise 5) wires all four earlier modules together without modifying them.

### Usage

1. Clone the repository and import each project into STM32CubeIDE via **File → Import → General → Existing Projects**.
2. Build and flash the relevant project to the STM32F3 Discovery Board.
3. For Exercise 5, flash `board1_sender` to the first board and `board2_receiver` to the second, then connect their UARTs (TX1 → RX2, common GND).
4. Open a serial terminal (e.g. PuTTY) at 115200 baud on USART1 for debug output.

### Valid input

- GPIO pin/port: any valid STM32F3 combination configured before use.
- Timer period: positive integer in milliseconds.
- Servo position: 0.0–1.0 (maps to 1–2 ms pulse width at 50 Hz).
- UART strings: null-terminated; structured messages use `sizeof(struct)` for length.
- Compass heading: 0.0–360.0 degrees output; no user input required.

### Functions and modularity

| Module | Key functions |
|---|---|
| GPIO (Ex 1) | `gpio_init`, `gpio_read`, `gpio_write`, `button_init`, `led_set`, `led_get` |
| Timer/PWM (Ex 2) | `timer_init`, `timer_set_period`, `timer_get_period`, `servo_set_position`, `timer_oneshot` |
| Serial (Ex 3) | `serial_init`, `serial_send_string`, `serial_send_msg`, ISR-driven receive with callback |
| Compass (Ex 4) | `compass_init`, `compass_read`, `compass_get_data`, `CompassData` struct |
| Integration (Ex 5) | Composes all above; Board 1 sends heading + button state; Board 2 drives servo or LEDs |

### Testing

- **Ex 1:** Toggle individual LEDs; confirm button callback fires once per press; verify `led_get` returns the value set by `led_set`.
- **Ex 2:** Verify 50 Hz PWM on oscilloscope; move servo to CW/centre/CCW positions; confirm one-shot fires exactly once.
- **Ex 3:** Check debug string appears correctly in terminal; send a structured packet and verify checksum and framing on PC; test buffer overflow handling.
- **Ex 4:** Rotate board toward N/E/S/W and verify heading matches; confirm timestamp increments; send compass struct over UART and decode on PC.
- **Ex 5:** Rotate Board 1 and confirm servo on Board 2 tracks heading; press button and confirm Board 2 switches to LED display with the correct LED lit.

### Notes

- All modules use function pointers for callbacks — keep callbacks short to avoid blocking interrupts.
- Meeting minutes and agendas are stored in the `minutes/` folder of this repository.
- Tag the repo as **"assessment demonstration"** before the week 8 lab session.

# MTRX2700 Project 2 — C Lab

## Group Members
| Name | SID | Role |
|---|---|---|
| William Kirk | 550667928 | Exercise 1 — Digital I/O |
| Josh Johnson | — | Exercise 2 — Timer / PWM |
| Hajin Paek | — | Exercise 3 — Serial Interface |
| Michael Deng | — | Exercise 4 — I2C Compass, Exercise 5 — Integration |

## Project Overview

This project implements five C-language software modules for the STM32F3 Discovery Board using the STM32 HAL and direct register access in C. It covers GPIO/LED/button drivers, hardware timer-based PWM for a hobby servo, interrupt-driven UART communication with structured packets and checksums, I2C interfacing with the on-board compass module, and a full integration task connecting two boards over UART to drive a servo and LED array from live magnetometer heading data.

## Hardware

- STM32F3 Discovery board (STM32F303VCT6) × 2
- Hobby servo (3-wire, 50 Hz PWM, 1–2 ms pulse)
- Jumper wires
- Optional: USB-UART adapter for PC serial monitor testing

---

## Task Overview

### Exercise 1 — Digital I/O

Implements a layered GPIO driver. A generic module configures any GPIO pin as input or output and provides read/write access. Two higher-level modules wrap the Discovery Board's user button (PA0) and 8-LED array (PE8–PE15).

**LEDs** — PE8–PE15 on GPIOE, controlled via HAL GPIO write  
**Button** — PA0, interrupt-driven via EXTI line with callback registration  
**LED state** — fully encapsulated; only accessible through `led_set()` / `led_get()`  
**Advanced** — LED change rate limited by a hardware timer interrupt; `led_set()` returns immediately

#### Functions

`gpio.c / gpio.h`
- `gpio_init(port, pin, mode)` — configure pin as input or output
- `gpio_write(port, pin, value)` — drive output pin high or low
- `gpio_read(port, pin)` — return current input pin state

`button.c / button.h`
- `button_init(callback)` — configure PA0 with EXTI interrupt and register press callback

`led.c / led.h`
- `led_init()` — configure PE8–PE15 as outputs
- `led_set(state)` — set LED bitmask (0–255); state is encapsulated
- `led_get()` — return current LED bitmask

#### Testing
- Toggle each LED individually and confirm the correct physical LED lights
- Press the button and confirm the callback fires exactly once per press
- Call `led_set(x)` then `led_get()` and verify the return value matches `x`
- Confirm LED state variable is not directly accessible outside the module

---

### Exercise 2 — Timer Interface / PWM

Implements a hardware timer software module that fires a user-supplied callback at a configurable interval. Builds on this to generate a 50 Hz PWM signal (20 ms period, 1–2 ms pulse width) for a hobby servo.

**Timer** — TIM2 used for general-purpose repeating and one-shot callbacks  
**Servo PWM** — 50 Hz output on a GPIO pin; pulse width 1 ms (CW) → 1.5 ms (centre) → 2 ms (CCW)  
**Advanced** — one-shot timer event: fires once, triggers callback, then disarms

#### Timer Design

| Use case | Timer | PSC | ARR | Resolution |
|---|---|---|---|---|
| Repeating callback | TIM2 | configurable | configurable | user-defined ms |
| PWM 50 Hz | TIM2 | 7999 | 19 | 1 ms tick, 20 ms period |
| One-shot delay | TIM2 | 7999 | delay_ms | 1 ms per tick |

#### Functions

`timer.c / timer.h`
- `timer_init(period_ms, callback)` — configure TIM2 as repeating timer with interrupt callback
- `timer_set_period(ms)` / `timer_get_period()` — encapsulated period access
- `timer_stop()` — halt the timer
- `timer_oneshot(delay_ms, callback)` — (Advanced) one-shot delayed event

`servo.c / servo.h`
- `servo_init(pin)` — configure output GPIO pin for PWM
- `servo_set_position(pos)` — `pos` 0.0–1.0 maps to 1–2 ms pulse; 0.5 = centre

#### Testing
- Verify 50 Hz output and correct pulse widths on an oscilloscope or logic analyser
- Set `servo_set_position(0.0)`, `(0.5)`, `(1.0)` and confirm physical arm positions
- Change period via `timer_set_period()` and confirm callback rate changes accordingly
- Trigger one-shot and confirm callback fires exactly once then stops

---

### Exercise 3 — Serial Interface

Implements a UART module with interrupt-driven receive, structured packet framing with checksum, a debug string sender over USART1, and (advanced) double-buffered receive.

**Packet format** — `[START | SIZE | TYPE | BODY... | CHECKSUM | STOP]`  
**Receive** — ISR-driven; callback triggered on terminating character  
**Debug** — `serial_send_string()` transmits to USART1 for PC console output  
**Advanced** — double buffer: one buffer fills while the other is consumed

#### Functions

`serial.c / serial.h`
- `serial_init(uart, baud, rx_callback)` — initialise UART peripheral and register receive callback
- `serial_send_string(str)` — transmit null-terminated string (debug, USART1)
- `serial_send_msg(type, ptr, size)` — assemble framed packet and transmit over given UART
- ISR handler accumulates bytes; triggers `rx_callback(buffer, length)` on terminating character

#### Testing
- Send a known string from `serial_send_string()` and confirm it appears correctly in a PC serial terminal
- Transmit a structured packet, capture it on the PC, and verify framing and checksum byte-by-byte
- Send a packet from the PC to the board and confirm the callback fires with correct data and byte count
- Send more bytes than `RX_BUFFER_SIZE` and confirm overflow is handled gracefully

#### Connecting to PC
```
screen /dev/tty.usbmodemXXXX 115200
```
Or use PuTTY on Windows at 115200 baud, connected to the USART1 COM port.

---

### Exercise 4 — I2C Compass / Magnetometer

Implements an I2C driver for the LSM303DLHC magnetometer built into the STM32F3 Discovery Board. Reads raw X/Y/Z magnetic field registers, decodes a compass heading, and stores all values with a timestamp in a shared structure.

**Sensor** — LSM303DLHC magnetometer via I2C1  
**Output** — decoded heading 0.0–360.0°, raw int16 X/Y/Z values, HAL tick timestamp  
**Advanced** — extends to accelerometer (same I2C device) and SPI gyro for full attitude estimation

#### Data Structure

```c
typedef struct {
    int16_t  raw_x;
    int16_t  raw_y;
    int16_t  raw_z;
    float    heading;      // degrees, 0.0–360.0
    uint32_t timestamp;    // HAL_GetTick() value at time of read
} CompassData;
```

#### Functions

`compass.c / compass.h`
- `compass_init()` — configure I2C1 peripheral and set magnetometer output rate/gain registers
- `compass_read()` — read XYZ registers over I2C, decode heading using `atan2f`, update struct and timestamp
- `compass_get_data()` — return pointer to latest `CompassData` (read-only for other modules)

#### Testing
- Point the board toward N, E, S, W and verify heading output is within ±15° of expected
- Confirm timestamp increments correctly between successive calls to `compass_read()`
- Serialise the struct over UART (Exercise 3) and decode raw values on the PC to verify correctness

---

### Exercise 5 — Integration Task

Connects all four modules. Board 1 reads compass heading and monitors the user button, packages the data into UART messages, and sends them to Board 2. Board 2 drives a hobby servo to track the heading and switches to an LED compass display when the button is pressed.

```
Board 1                          Board 2
--------                         --------
Magnetometer (I2C)               Servo (PWM)
Button (EXTI)       --UART-->    LED array (GPIO)
Debug console (USART1)           Debug console (USART1)
```

**Board 1 TX → Board 2 RX** (connect PC10 → PC11 on second board, share GND)

#### Board 1 Behaviour
- Calls `compass_read()` in main loop; packages `CompassData` into a serial message via `serial_send_msg()`
- Button EXTI callback sets a `display_mode` flag included in each outgoing message
- Debug strings sent over USART1 for monitoring on PC

#### Board 2 Behaviour
- UART receive callback fires on complete message
- Default mode: calls `servo_set_position(heading / 360.0f)` to track heading
- When `display_mode` flag is set: lights one of the 8 LEDs corresponding to the heading octant (each LED covers 45°)

#### Heading → LED Mapping

| Heading range | LED (PE pin) | Direction |
|---|---|---|
| 337.5° – 22.5° | PE8 | North |
| 22.5° – 67.5° | PE9 | North-East |
| 67.5° – 112.5° | PE10 | East |
| 112.5° – 157.5° | PE11 | South-East |
| 157.5° – 202.5° | PE12 | South |
| 202.5° – 247.5° | PE13 | South-West |
| 247.5° – 292.5° | PE14 | West |
| 292.5° – 337.5° | PE15 | North-West |

#### Testing
- Test each board independently using `serial_send_string()` debug output before connecting them together
- Rotate Board 1 and verify the servo on Board 2 tracks the heading in real time
- Press the button and confirm Board 2 switches from servo to LED display
- Confirm the correct LED lights for each cardinal direction

---

## Building and Flashing

1. Clone this repository and open STM32CubeIDE
2. Import projects: **File → Import → General → Existing Projects into Workspace**, select repo root
3. Select the exercise project to build, then **Project → Build All**
4. Connect the STM32F3 Discovery board via USB and click **Run → Debug** or **Run As → STM32 C/C++ Application**
5. For Exercise 5: build and flash `Exercise5_Board1` to the sender board and `Exercise5_Board2` to the receiver board

## Repository Structure

```
/
├── Exercise1_DigitalIO/
├── Exercise2_Timer/
├── Exercise3_Serial/
├── Exercise4_Compass/
├── Exercise5_Integration/
│   ├── Board1_Sender/
│   └── Board2_Receiver/
├── minutes/
└── README.md
```

## Meeting Minutes

All group meeting agendas and minutes are stored in the `minutes/` folder.

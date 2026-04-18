# Exercise 4 & 5 — Deep Dive
## I2C Compass Interfacing and Full Integration Task

---

## What is I2C?

**I2C (Inter-Integrated Circuit)** is a two-wire serial communication protocol used to connect a microcontroller to peripheral chips — sensors, displays, EEPROMs, etc. It was designed by Philips in the 1980s and is still one of the most widely used embedded protocols.

### The two wires

| Wire | Name | Purpose |
|---|---|---|
| **SCL** | Serial Clock | The master drives this to set the communication speed |
| **SDA** | Serial Data | Bidirectional data line — both master and slave use it |

Both lines are **open-drain** with **pull-up resistors** to VCC (typically 4.7 kΩ). A device pulls the line LOW to assert it. Neither device ever drives the line HIGH — they just release it and the resistor pulls it up.

On the STM32F3 Discovery board: **SCL = PB6**, **SDA = PB7**.

---

### How a transaction works

Every I2C transaction follows this sequence:

```
START → ADDRESS + R/W bit → ACK → DATA byte(s) → ACK → ... → STOP
```

**Step by step:**

1. **START condition** — master pulls SDA low while SCL is high. This signals all devices on the bus that a transaction is beginning.

2. **7-bit address + R/W bit** — master clocks out 8 bits: the 7-bit address of the target device, then a direction bit (`0` = write, `1` = read). All devices on the bus hear this but only the one with the matching address responds.

3. **ACK** — the addressed slave pulls SDA low for one clock pulse to say "I'm here, go ahead." If no device acknowledges, the master knows no device exists at that address.

4. **Data bytes** — one byte at a time, clocked by SCL. After each byte the receiver sends an ACK bit. For a write, the master sends and the slave ACKs. For a read, the slave sends and the master ACKs (or sends NACK on the last byte to signal it is done).

5. **STOP condition** — master releases SDA while SCL is high. Bus goes idle.

**Write example (setting a register):**
```
START → [ADDR W] → ACK → [REG] → ACK → [VALUE] → ACK → STOP
```

**Read example (reading a register):**
```
START → [ADDR W] → ACK → [REG] → ACK → RESTART → [ADDR R] → ACK → [DATA] → NACK → STOP
```

---

### I2C addresses

Every I2C device has a fixed 7-bit address (set by the chip manufacturer, sometimes configurable via address pins). Multiple devices can share the same two wires as long as they have different addresses.

The STM32 HAL uses **8-bit addresses** (the 7-bit address shifted 1 bit left). So when a datasheet says the address is `0x1E`, you pass `0x1E << 1 = 0x3C` to HAL functions.

| Device on Discovery board | 7-bit address | HAL 8-bit address |
|---|---|---|
| LSM303DLHC Magnetometer | `0x1E` | `0x3C` |
| LSM303DLHC Accelerometer | `0x19` | `0x32` |

---

### I2C in the STM32 HAL

The HAL gives you two key functions:

```c
// Write bytes TO a device (e.g. set a register)
HAL_StatusTypeDef HAL_I2C_Master_Transmit(
    I2C_HandleTypeDef *hi2c,   // e.g. &hi2c1
    uint16_t DevAddress,       // 8-bit address (7-bit << 1)
    uint8_t *pData,            // pointer to bytes to send
    uint16_t Size,             // number of bytes
    uint32_t Timeout           // ms before giving up (use HAL_MAX_DELAY)
);

// Read bytes FROM a device (e.g. read a sensor register)
HAL_StatusTypeDef HAL_I2C_Master_Receive(
    I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress,
    uint8_t *pData,            // buffer to read into
    uint16_t Size,
    uint32_t Timeout
);
```

Both return `HAL_OK` (0) on success, `HAL_ERROR` or `HAL_TIMEOUT` on failure.

**Writing one register:**
```c
uint8_t buf[2] = { reg_address, value };
HAL_I2C_Master_Transmit(&hi2c1, 0x3C, buf, 2, HAL_MAX_DELAY);
```

**Reading multiple registers starting at a given address:**
```c
uint8_t reg = 0x03;
uint8_t data[6];
HAL_I2C_Master_Transmit(&hi2c1, 0x3C, &reg, 1, 100);  // tell sensor which register
HAL_I2C_Master_Receive(&hi2c1,  0x3C, data, 6, 100);  // read 6 bytes back
```

---

### Testing I2C independently

Before writing any sensor driver, verify the I2C bus is alive with an **I2C scanner**. Paste this into `main()` after `MX_I2C1_Init()`:

```c
char s[40];
for (uint8_t addr = 1; addr < 128; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 2, 10) == HAL_OK) {
        int len = sprintf(s, "I2C device found at 0x%02X\r\n", addr);
        HAL_UART_Transmit(&huart1, (uint8_t *)s, len, 100);
    }
}
HAL_UART_Transmit(&huart1, (uint8_t *)"Scan done\r\n", 11, 100);
```

Expected output on the PC serial terminal:
```
I2C device found at 0x19
I2C device found at 0x1E
Scan done
```

If you see nothing, the I2C peripheral is not initialised or the pull-up resistors are missing (the Discovery board has them built-in, so this usually means a CubeMX setup error).

**Testing a single register write/read:**
```c
// Write 0x00 (continuous mode) to MR_REG_M (0x02) of the magnetometer
uint8_t cmd[2] = { 0x02, 0x00 };
HAL_StatusTypeDef w = HAL_I2C_Master_Transmit(&hi2c1, 0x3C, cmd, 2, 100);

// Read it back
uint8_t reg = 0x02, val = 0;
HAL_I2C_Master_Transmit(&hi2c1, 0x3C, &reg, 1, 100);
HAL_I2C_Master_Receive(&hi2c1,  0x3C, &val, 1, 100);

char s[40];
sprintf(s, "write=%d  MR_REG_M readback=0x%02X\r\n", w, val);
HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), 100);
// Expected: write=0  MR_REG_M readback=0x00
```

**Checking HAL return codes:**

| Code | Meaning |
|---|---|
| `HAL_OK` (0) | Transaction succeeded |
| `HAL_ERROR` (1) | Bus error or no ACK — wrong address or wiring issue |
| `HAL_BUSY` (2) | I2C peripheral already in use |
| `HAL_TIMEOUT` (3) | Device did not respond in time |

---

## Exercise 4 — I2C Compass / Magnetometer

### Overview

The STM32F3 Discovery board has an **LSM303DLHC** sensor soldered directly onto it. This chip contains both a 3-axis magnetometer and a 3-axis accelerometer on I2C1 (PB6 = SCL, PB7 = SDA). You will read the magnetometer to get raw magnetic field strength on X, Y, Z axes, then compute a 2D compass heading using `atan2`.

---

### 4.1 — Understanding the LSM303DLHC

The chip has two separate I2C addresses:

| Sensor | 7-bit address | HAL 8-bit address (shift left 1) |
|---|---|---|
| Accelerometer | `0x19` | `0x32` |
| Magnetometer | `0x1E` | `0x3C` |

**Key magnetometer registers:**

| Register | Hex | What it does |
|---|---|---|
| `CRA_REG_M` | `0x00` | Output data rate (set `0x10` = 15 Hz) |
| `CRB_REG_M` | `0x01` | Gain / full-scale (set `0x20` = ±1.3 Gauss) |
| `MR_REG_M`  | `0x02` | Mode: `0x00` = continuous, `0x01` = single, `0x03` = sleep |
| `OUT_X_H_M` | `0x03` | Start of 6-byte output block |

Reading starts at `0x03` and returns 6 bytes in this order: `X_H, X_L, Z_H, Z_L, Y_H, Y_L`

> **Gotcha:** The register order from the sensor is X, Z, Y — **not** X, Y, Z. This is a documented quirk of the LSM303DLHC.

---

### 4.2 — CubeMX Setup (step by step)

1. Open STM32CubeIDE → File → New → STM32 Project → `STM32F303VCTx`
2. **Connectivity → I2C1**
   - Mode: `I2C`
   - Speed Mode: `Standard Mode` (100 kHz)
   - SCL pin auto-assigned to PB6, SDA to PB7
3. **Connectivity → USART1** (for debug)
   - Mode: `Asynchronous`
   - Baud: `115200`, Word Length: `8 Bits`, No parity, 1 stop bit
   - PA9 = TX, PA10 = RX
4. **Project Manager** → set project name → Generate Code

---

### 4.3 — Full compass module code

#### `compass.h`
```c
#ifndef COMPASS_H
#define COMPASS_H

#include <stdint.h>

typedef struct {
    int16_t  raw_x;
    int16_t  raw_y;
    int16_t  raw_z;
    float    heading;     // 0.0 to 360.0 degrees
    uint32_t timestamp;   // milliseconds since boot (HAL_GetTick)
} CompassData;

void         compass_init(void);
void         compass_read(void);
CompassData *compass_get_data(void);

#endif
```

#### `compass.c`
```c
#include "compass.h"
#include "stm32f3xx_hal.h"
#include <math.h>

/* ---- Register addresses ---- */
#define MAG_ADDR        (0x1E << 1)   // 8-bit HAL address
#define MAG_CRA_REG     0x00
#define MAG_CRB_REG     0x01
#define MAG_MR_REG      0x02
#define MAG_OUT_X_H     0x03

extern I2C_HandleTypeDef hi2c1;

static CompassData compass_data = {0};

/* Write one byte to a magnetometer register */
static void mag_write(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        &hi2c1, MAG_ADDR, buf, 2, 100);
    (void)status;  // check status in debug builds if reads return zeros
}

/* Read `len` bytes starting from `reg` into `out` */
static void mag_read(uint8_t reg, uint8_t *out, uint16_t len) {
    HAL_I2C_Master_Transmit(&hi2c1, MAG_ADDR, &reg, 1, 100);
    HAL_I2C_Master_Receive(&hi2c1,  MAG_ADDR, out, len, 100);
}

/* ---- Public functions ---- */

void compass_init(void) {
    HAL_Delay(20);                         // sensor needs time after power-on
    mag_write(MAG_CRA_REG, 0x10);          // 15 Hz output rate
    mag_write(MAG_CRB_REG, 0x20);          // ±1.3 Gauss gain
    mag_write(MAG_MR_REG,  0x00);          // continuous measurement mode
}

void compass_read(void) {
    uint8_t raw[6];
    mag_read(MAG_OUT_X_H, raw, 6);

    // Register order is X_H, X_L, Z_H, Z_L, Y_H, Y_L (NOT X Y Z)
    compass_data.raw_x = (int16_t)((raw[0] << 8) | raw[1]);
    compass_data.raw_z = (int16_t)((raw[2] << 8) | raw[3]);
    compass_data.raw_y = (int16_t)((raw[4] << 8) | raw[5]);

    // 2D heading — assumes board is held flat (horizontal)
    float angle = atan2f((float)compass_data.raw_y, (float)compass_data.raw_x);
    float degrees = angle * (180.0f / (float)M_PI);
    if (degrees < 0.0f) degrees += 360.0f;

    compass_data.heading   = degrees;
    compass_data.timestamp = HAL_GetTick();
}

CompassData *compass_get_data(void) {
    return &compass_data;
}
```

---

### 4.4 — main.c for Exercise 4

```c
#include "main.h"
#include "compass.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;
extern I2C_HandleTypeDef  hi2c1;

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();

    compass_init();

    char msg[80];
    while (1) {
        compass_read();
        CompassData *d = compass_get_data();

        int len = sprintf(msg,
            "Heading: %6.1f | X:%6d Y:%6d Z:%6d | T:%lu ms\r\n",
            d->heading, d->raw_x, d->raw_y, d->raw_z, d->timestamp);

        HAL_UART_Transmit(&huart1, (uint8_t *)msg, len, HAL_MAX_DELAY);
        HAL_Delay(200);
    }
}
```

---

### 4.5 — Running and verifying Exercise 4

**Setup:**
1. Build project and flash to Discovery board
2. Open serial terminal on your PC at **115200 baud** on the USART1 port
   - macOS: `screen /dev/tty.usbmodemXXXX 115200`
   - Windows: PuTTY → Serial → COM port → 115200

**What you should see:**
```
Heading:   23.4 | X:   312 Y:   127 Z:  -856 | T:200 ms
Heading:   24.1 | X:   314 Y:   129 Z:  -858 | T:400 ms
```

**Verification tests:**

| Test | Action | Expected |
|---|---|---|
| Data flowing | Run code flat on desk | Heading prints every ~200 ms, values change slightly |
| North | Point the board so the USB connector faces magnetic north | Heading near 0° or 360° |
| East | Rotate 90° clockwise from North | Heading near 90° |
| South | Rotate 180° | Heading near 180° |
| West | Rotate 270° | Heading near 270° |
| Timestamp | Watch T field | Increases by ~200 each line |
| Zeros / garbage | Check raw_x raw_y raw_z | Should be in range ~±2000 if sensor is working |

**Troubleshooting:**

| Symptom | Cause | Fix |
|---|---|---|
| All zeros | I2C not working | Check PB6/PB7 wiring; verify I2C1 is enabled in CubeMX |
| Heading stuck | Sensor in wrong mode | Call `compass_init()` before `compass_read()` |
| Heading wildly wrong | Board not flat | For 2D heading, board must be horizontal |
| `HAL_I2C_Master_Transmit` returns `HAL_ERROR` | Wrong I2C address | Try `0x3C` vs `0x3D`; run I2C scanner |

**I2C Scanner (paste into main to check address):**
```c
for (uint8_t addr = 0; addr < 128; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 1, 10) == HAL_OK) {
        char s[32];
        sprintf(s, "Found device at 0x%02X\r\n", addr);
        HAL_UART_Transmit(&huart1, (uint8_t *)s, strlen(s), 100);
    }
}
```
Expected output: `Found device at 0x1E` (magnetometer) and `Found device at 0x19` (accelerometer).

---

### 4.6 — Linking math.h

If the build fails on `atan2f` or `M_PI`:

1. Project → Properties → C/C++ Build → Settings
2. MCU GCC Linker → Libraries
3. Click **+** and add: `m`
4. Rebuild

---

## Exercise 5 — Integration Task

### Overview

Two Discovery boards communicate over UART4 (PC10/PC11). Board 1 reads the compass and button; Board 2 drives a servo and LEDs. Every module from Exercises 1–4 is reused as-is.

---

### 5.1 — Physical wiring

```
Board 1 PC10 (UART4 TX) ────────────────► Board 2 PC11 (UART4 RX)
Board 1 GND             ────────────────── Board 2 GND

Board 2 PA6 ──────────────────────────────► Servo signal (orange / white wire)
Board 2 5V or 3.3V ───────────────────────► Servo power (red wire)
Board 2 GND ──────────────────────────────► Servo ground (brown / black wire)
```

> Servos typically need 4.8–6 V. If powering from the board 3.3 V pin, some servos may not move. Use an external 5 V supply sharing GND with the board if needed.

---

### 5.2 — Shared message definition

Create `Core/Inc/messages.h` — **copy this file to both board projects**:

```c
#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

#define MSG_TYPE_COMPASS  0x01   // message type identifier

typedef struct __attribute__((packed)) {
    float    heading;        // 0.0 to 360.0 degrees
    int16_t  raw_x;
    int16_t  raw_y;
    int16_t  raw_z;
    uint32_t timestamp;
    uint8_t  display_mode;  // 0 = servo mode, 1 = LED mode
} CompassMsg;

#endif
```

> `__attribute__((packed))` ensures no padding bytes — safe to send/receive raw over UART.

---

### 5.3 — Board 1 CubeMX setup

1. Start from the Exercise 4 project (already has I2C1 and USART1)
2. Add **Connectivity → UART4**:
   - Mode: `Asynchronous`, Baud: `115200`
   - PC10 = TX, PC11 = RX
3. **System Core → GPIO → PA0**: `GPIO_EXTI0`, rising edge trigger
4. **NVIC**: enable `EXTI line0 interrupt`
5. Generate code

---

### 5.4 — Board 1 full code

#### `Core/Src/main.c` (Board 1)
```c
#include "main.h"
#include "compass.h"
#include "button.h"
#include "serial.h"
#include "messages.h"
#include <stdio.h>
#include <string.h>

extern UART_HandleTypeDef huart1;   // USART1 — debug console
extern UART_HandleTypeDef huart4;   // UART4  — to Board 2

static volatile uint8_t display_mode = 0;   // toggled by button

/* Button callback — called from EXTI interrupt on PA0 press */
static void on_button_press(void) {
    display_mode ^= 1;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USART1_UART_Init();
    MX_UART4_Init();

    compass_init();
    button_init(on_button_press);
    serial_init(&huart4, NULL);   // transmit only on Board 1

    serial_send_string(&huart1, "Board 1 ready\r\n");

    char debug[80];
    while (1) {
        compass_read();
        CompassData *d = compass_get_data();

        CompassMsg msg;
        msg.heading      = d->heading;
        msg.raw_x        = d->raw_x;
        msg.raw_y        = d->raw_y;
        msg.raw_z        = d->raw_z;
        msg.timestamp    = d->timestamp;
        msg.display_mode = display_mode;

        serial_send_msg(&huart4, MSG_TYPE_COMPASS, &msg, sizeof(CompassMsg));

        sprintf(debug, "TX heading=%.1f mode=%d\r\n", msg.heading, display_mode);
        serial_send_string(&huart1, debug);

        HAL_Delay(100);   // send 10 updates per second
    }
}
```

---

### 5.5 — Board 2 CubeMX setup

1. New project — `STM32F303VCTx`
2. **PE8–PE15**: `GPIO_Output` (LEDs)
3. **PA6**: `GPIO_Output` (servo PWM pin)
4. **Connectivity → USART1**: `Asynchronous`, 115200 baud
5. **Connectivity → UART4**: `Asynchronous`, 115200 baud, NVIC interrupt enabled
6. **Timers → TIM2**: Internal Clock, Prescaler = `7999`, Counter Period = `19`, ARPE enabled, NVIC interrupt enabled
7. Generate code

---

### 5.6 — Board 2 full code

#### `Core/Src/main.c` (Board 2)
```c
#include "main.h"
#include "led.h"
#include "servo.h"
#include "serial.h"
#include "messages.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart4;

/* Called when a complete, validated packet arrives over UART4 */
static void on_receive(uint8_t *buf, uint16_t len) {
    // Packet layout: [0xAA | size | type | body... | checksum | 0x55]
    if (len < (sizeof(CompassMsg) + 5)) return;   // minimum valid length check
    if (buf[2] != MSG_TYPE_COMPASS) return;

    CompassMsg msg;
    memcpy(&msg, &buf[3], sizeof(CompassMsg));

    if (msg.display_mode == 0) {
        /* --- Servo mode --- */
        // Map heading 0–360 to servo position 0.0–1.0
        float pos = msg.heading / 360.0f;
        servo_set_position(pos);
        led_set(0x00);   // all LEDs off

    } else {
        /* --- LED compass mode --- */
        servo_set_position(0.5f);   // centre servo

        // Map heading to one of 8 LEDs (each LED = 45° sector)
        // Add 22.5 so 0° centres on octant 0 (North)
        uint8_t octant = (uint8_t)((msg.heading + 22.5f) / 45.0f) % 8;
        led_set(1 << octant);
    }

    char debug[64];
    sprintf(debug, "RX heading=%.1f mode=%d\r\n", msg.heading, msg.display_mode);
    serial_send_string(&huart1, debug);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_UART4_Init();
    MX_TIM2_Init();

    led_init();
    servo_init();
    serial_init(&huart4, on_receive);

    serial_send_string(&huart1, "Board 2 ready\r\n");

    while (1) {
        // All processing is interrupt-driven — main loop stays empty
    }
}
```

---

### 5.7 — Heading to LED octant mapping explained

```
         PE8 (N)
    PE15 /      \ PE9 (NE)
   (NW) |        |
        |        |
   PE14  \      / PE10 (E → actually SE if using geographic convention)
   (W)    ------
        PE13  PE11
        (SW)  (S)  PE12 (SE)
```

The `octant` formula:
```c
uint8_t octant = (uint8_t)((msg.heading + 22.5f) / 45.0f) % 8;
```
| Octant | Heading range | LED | Meaning |
|---|---|---|---|
| 0 | 337.5° – 22.5° | PE8 | North |
| 1 | 22.5° – 67.5° | PE9 | North-East |
| 2 | 67.5° – 112.5° | PE10 | East |
| 3 | 112.5° – 157.5° | PE11 | South-East |
| 4 | 157.5° – 202.5° | PE12 | South |
| 5 | 202.5° – 247.5° | PE13 | South-West |
| 6 | 247.5° – 292.5° | PE14 | West |
| 7 | 292.5° – 337.5° | PE15 | North-West |

---

### 5.8 — Step-by-step run procedure

1. **Flash Board 1** with the Board 1 project
2. **Flash Board 2** with the Board 2 project
3. **Disconnect both boards** from the PC USB (or keep connected for debug)
4. **Connect UART wire**: Board 1 PC10 → Board 2 PC11
5. **Connect GND wire** between both boards
6. **Connect servo** to Board 2: PA6 (signal), 5V (power), GND
7. Open two serial terminals (one per board USART1 port) at 115200 baud
8. Power on both boards
9. Board 1 terminal should print: `Board 1 ready` then `TX heading=... mode=0`
10. Board 2 terminal should print: `Board 2 ready` then `RX heading=... mode=0`
11. Servo on Board 2 should begin moving as you rotate Board 1

---

### 5.9 — Full verification checklist

| Test | Steps | Pass condition |
|---|---|---|
| UART comms working | Run both boards, watch Board 2 terminal | Board 2 prints RX messages |
| Servo tracks heading | Rotate Board 1 slowly 360° | Servo arm sweeps correspondingly |
| Servo at North (0°) | Point Board 1 toward North | Servo near 1 ms (full CW) |
| Servo at South (180°) | Point Board 1 toward South | Servo near 2 ms (full CCW) |
| Servo at East (90°) | Point Board 1 toward East | Servo at ~1.25 ms (quarter position) |
| Button toggles mode | Press button on Board 1 | Board 2 switches to LED display |
| LED North | LED mode active, point North | PE8 only lights up |
| LED East | LED mode active, point East | PE10 only lights up |
| LED South | LED mode active, point South | PE12 only lights up |
| Return to servo | Press button again | LEDs off, servo resumes tracking |
| Disconnect UART | Pull wire while running | Board 2 holds last position, no crash |

---

### 5.10 — Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| Board 2 gets no data | Wrong wire or GND not shared | Verify PC10 → PC11 and GND-GND |
| Servo doesn't move | PWM not reaching servo | Check PA6 → servo signal wire; verify 20 ms period |
| Servo buzzes and doesn't hold | Pulse width out of range | Clamp position 0.0–1.0 |
| All LEDs on / wrong LED | Octant calculation off | Print octant value over USART1 and compare to heading |
| Checksum fail | Struct padding bytes differ between boards | Use `__attribute__((packed))` on CompassMsg |
| Nothing on terminal | USART1 wrong port | Check which COM port each board appears as |
| Board 1 prints but Board 2 doesn't receive | TX and RX swapped | Try swapping the UART wire ends |

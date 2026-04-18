#include "main.h"
#include "compass.h"
#include <math.h>

/* LSM303AGR magnetometer
   7-bit I2C address: 0x1E
   HAL uses 8-bit addresses (shifted left by 1): 0x3C */
#define MAG_ADDR         0x3C

/* LSM303AGR magnetometer register addresses */
#define MAG_CFG_REG_A    0x60   /* Configuration register: set continuous mode */
#define MAG_OUT_X_L      0x68   /* First output byte (X low), read 6 bytes total */

/* hi2c1 is declared in main.c by CubeMX */
extern I2C_HandleTypeDef hi2c1;

/* Internal data - private to this module. Access via compass_get_data() only */
static CompassData compass_data = {0};

/* Initialise the LSM303AGR magnetometer */
void compass_init(void) {
    HAL_Delay(20);  /* Wait for sensor power-up */

    /* Write 0x00 to CFG_REG_A_M:
       continuous measurement mode, 10Hz output data rate */
    uint8_t val = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MAG_ADDR, MAG_CFG_REG_A,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);

    HAL_Delay(20);  /* Wait for first measurement to be ready */
}

/* Read magnetometer, calculate heading, store with timestamp */
void compass_read(void) {
    uint8_t raw[6] = {0};

    /* Read 6 bytes starting at OUTX_L_REG_M (0x68)
       Byte order is little-endian: X_L, X_H, Y_L, Y_H, Z_L, Z_H */
    if (HAL_I2C_Mem_Read(&hi2c1, MAG_ADDR, MAG_OUT_X_L,
                         I2C_MEMADD_SIZE_8BIT, raw, 6, 100) != HAL_OK) {
        return;  /* I2C error - keep previous values */
    }

    /* Combine low and high bytes into signed 16-bit integers */
    compass_data.raw_x = (int16_t)(raw[0] | (raw[1] << 8));
    compass_data.raw_y = (int16_t)(raw[2] | (raw[3] << 8));
    compass_data.raw_z = (int16_t)(raw[4] | (raw[5] << 8));

    /* Calculate 2D heading (assumes board is held flat)
       atan2f handles all 4 quadrants correctly (unlike atan)
       Result is in radians, multiply by 180/pi to get degrees */
    float angle = atan2f((float)compass_data.raw_y, (float)compass_data.raw_x)
                  * (180.0f / 3.14159265f);

    /* atan2f returns -180 to +180, shift to 0 to 360 */
    if (angle < 0.0f) angle += 360.0f;

    compass_data.heading   = angle;
    compass_data.timestamp = HAL_GetTick();
}

/* Return pointer to the latest compass data */
CompassData* compass_get_data(void) {
    return &compass_data;
}

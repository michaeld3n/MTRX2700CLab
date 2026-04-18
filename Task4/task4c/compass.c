#include "main.h"
#include "compass.h"
#include <math.h>

/* ---------------------------------------------------------------
   LSM303AGR Magnetometer
   7-bit address: 0x1E  ->  HAL 8-bit: 0x3C
   --------------------------------------------------------------- */
#define MAG_ADDR        0x3C
#define MAG_CFG_REG_A   0x60    /* Config: set continuous mode here */
#define MAG_OUT_X_L     0x68    /* First output byte - read 6 bytes X Y Z */

/* ---------------------------------------------------------------
   LSM303AGR Accelerometer
   7-bit address: 0x19  ->  HAL 8-bit: 0x32
   --------------------------------------------------------------- */
#define ACC_ADDR        0x32
#define ACC_CTRL_REG1   0x20    /* Control: enable axes and set output rate */
#define ACC_OUT_X_L     0x28    /* First output byte - read 6 bytes X Y Z */

/* Auto-increment bit for accelerometer multi-byte reads */
#define ACC_AUTO_INC    0x80

/* hi2c1 declared in main.c by CubeMX */
extern I2C_HandleTypeDef hi2c1;

/* Private data store - only accessible via compass_get_data() */
static CompassData compass_data = {0};

/* ---------------------------------------------------------------
   compass_init
   Call once after MX_I2C1_Init() in USER CODE BEGIN 2
   --------------------------------------------------------------- */
void compass_init(void) {
    HAL_Delay(20);

    /* --- Magnetometer init ---
       Write 0x00 to CFG_REG_A_M (0x60):
       bits [1:0] = 00 -> continuous measurement mode
       bits [3:2] = 00 -> 10 Hz output data rate
       All other bits 0 = normal mode, no offset cancellation */
    uint8_t val = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MAG_ADDR, MAG_CFG_REG_A,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);

    /* --- Accelerometer init ---
       Write 0x57 to CTRL_REG1_A (0x20):
       bits [7:4] = 0101 -> 100 Hz output data rate
       bit  [3]   = 0    -> normal (not low-power) mode
       bits [2:0] = 111  -> Z, Y, X axes all enabled */
    val = 0x57;
    HAL_I2C_Mem_Write(&hi2c1, ACC_ADDR, ACC_CTRL_REG1,
                      I2C_MEMADD_SIZE_8BIT, &val, 1, 100);

    HAL_Delay(20);
}

/* ---------------------------------------------------------------
   compass_read
   Reads both sensors, calculates tilt-compensated heading,
   pitch, and roll. Call in the while loop.
   --------------------------------------------------------------- */
void compass_read(void) {
    uint8_t raw[6] = {0};

    /* --- Read magnetometer (6 bytes, little-endian X Y Z) --- */
    if (HAL_I2C_Mem_Read(&hi2c1, MAG_ADDR, MAG_OUT_X_L,
                         I2C_MEMADD_SIZE_8BIT, raw, 6, 100) != HAL_OK) {
        return;
    }
    compass_data.raw_x = (int16_t)(raw[0] | (raw[1] << 8));
    compass_data.raw_y = (int16_t)(raw[2] | (raw[3] << 8));
    compass_data.raw_z = (int16_t)(raw[4] | (raw[5] << 8));

    /* --- Read accelerometer (6 bytes, little-endian X Y Z) ---
       Bit 7 of register address set to 1 enables auto-increment
       so all 6 output registers are read in one transaction */
    if (HAL_I2C_Mem_Read(&hi2c1, ACC_ADDR, ACC_OUT_X_L | ACC_AUTO_INC,
                         I2C_MEMADD_SIZE_8BIT, raw, 6, 100) != HAL_OK) {
        return;
    }
    /* Accelerometer data is left-justified 12-bit in a 16-bit register.
       Shift right by 4 to get the actual value. */
    compass_data.acc_x = (int16_t)(raw[0] | (raw[1] << 8)) >> 4;
    compass_data.acc_y = (int16_t)(raw[2] | (raw[3] << 8)) >> 4;
    compass_data.acc_z = (int16_t)(raw[4] | (raw[5] << 8)) >> 4;

    /* --- Calculate pitch and roll from accelerometer ---
       The accelerometer measures gravity (pointing straight down).
       When the board is flat: acc_z is large, acc_x and acc_y are near zero.
       When tilted, the gravity vector splits across the axes.
       pitch_rad: rotation around Y axis (nose up/down)
       roll_rad:  rotation around X axis (left/right tilt) */
    float ax = (float)compass_data.acc_x;
    float ay = (float)compass_data.acc_y;
    float az = (float)compass_data.acc_z;

    float pitch_rad = atan2f(-ax, sqrtf(ay * ay + az * az));
    float roll_rad  = atan2f(ay, az);

    compass_data.pitch = pitch_rad * (180.0f / 3.14159265f);
    compass_data.roll  = roll_rad  * (180.0f / 3.14159265f);

    /* --- Tilt-compensated heading ---
       When the board is tilted, raw X and Y mag values are wrong because
       the Z magnetic component bleeds into them.
       We rotate the magnetic vector by pitch and roll to get the true
       horizontal component, then calculate heading from that. */
    float mx = (float)compass_data.raw_x;
    float my = (float)compass_data.raw_y;
    float mz = (float)compass_data.raw_z;

    float cos_pitch = cosf(pitch_rad);
    float sin_pitch = sinf(pitch_rad);
    float cos_roll  = cosf(roll_rad);
    float sin_roll  = sinf(roll_rad);

    /* Project magnetic field onto the horizontal plane */
    float Xh = mx * cos_pitch + mz * sin_pitch;
    float Yh = mx * sin_roll * sin_pitch
             + my * cos_roll
             - mz * sin_roll * cos_pitch;

    float angle = atan2f(-Yh, Xh) * (180.0f / 3.14159265f);
    if (angle < 0.0f) angle += 360.0f;

    compass_data.heading   = angle;
    compass_data.timestamp = HAL_GetTick();
}

/* ---------------------------------------------------------------
   compass_get_data
   Returns a pointer to the latest CompassData.
   Call after compass_read() to access values.
   --------------------------------------------------------------- */
CompassData* compass_get_data(void) {
    return &compass_data;
}

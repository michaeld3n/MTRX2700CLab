#ifndef COMPASS_H
#define COMPASS_H

#include <stdint.h>

/* Full compass data structure including accelerometer, pitch, roll */
typedef struct {
    /* Raw magnetometer values (from LSM303AGR mag at 0x1E) */
    int16_t  raw_x;
    int16_t  raw_y;
    int16_t  raw_z;

    /* Raw accelerometer values (from LSM303AGR accel at 0x19) */
    int16_t  acc_x;
    int16_t  acc_y;
    int16_t  acc_z;

    /* Decoded angles */
    float    heading;   /* 0 to 360 degrees - tilt-compensated compass bearing */
    float    pitch;     /* degrees - positive = nose up */
    float    roll;      /* degrees - positive = right side tilts up */

    /* Time the reading was taken (ms since boot, from HAL_GetTick) */
    uint32_t timestamp;
} CompassData;

/* Initialise both magnetometer and accelerometer - call once at startup */
void compass_init(void);

/* Read both sensors, calculate heading/pitch/roll, update internal struct */
void compass_read(void);

/* Return pointer to latest data - call after compass_read() */
CompassData* compass_get_data(void);

#endif /* COMPASS_H */

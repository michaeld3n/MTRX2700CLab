#ifndef COMPASS_H
#define COMPASS_H

#include <stdint.h>

/* Structure to hold all compass data - shareable with other modules */
typedef struct {
    int16_t  raw_x;       /* Raw magnetic field X axis */
    int16_t  raw_y;       /* Raw magnetic field Y axis */
    int16_t  raw_z;       /* Raw magnetic field Z axis */
    float    heading;     /* Decoded compass heading, 0.0 to 360.0 degrees */
    uint32_t timestamp;   /* Time of reading in ms (from HAL_GetTick) */
} CompassData;

/* Initialise the magnetometer over I2C - call once at startup */
void compass_init(void);

/* Read latest data from magnetometer, update internal struct */
void compass_read(void);

/* Get pointer to the latest CompassData (read only) */
CompassData* compass_get_data(void);

#endif /* COMPASS_H */

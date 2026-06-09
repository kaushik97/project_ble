#include "driver/temperature_sensor.h"

typedef enum {
    OK=0,
    ERR_NULL_PTR,
    ERR_SENSOR_FAULT
} errorType_e;


int temperature_init(void);
void temperature_task(void *param);
errorType_e get_temperature(int32_t *temp);
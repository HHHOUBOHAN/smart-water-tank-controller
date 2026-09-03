#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool ultrasonic_valid;
    bool pressure_valid;
    bool pressure_calibrated;
    bool all_valid;
    uint32_t distance_mm;
    uint8_t level_percent;
    int32_t pressure_raw;
    int32_t pressure_delta_raw;
    int32_t pressure_kpa;
    uint32_t ultrasonic_last_update_ms;
    uint32_t pressure_last_update_ms;
} SensorServiceData_t;

void SensorService_Init(uint32_t now_ms);
void SensorService_Process(uint32_t now_ms);
const SensorServiceData_t *SensorService_GetData(void);
void SensorService_CapturePressureBaseline(void);
bool SensorService_IsPressureCalibrated(void);

#endif /* SENSOR_SERVICE_H */

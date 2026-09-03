#include "SERVICE/sensor_service.h"

#include "BSP/hx710b.h"
#include "BSP/ultrasonic.h"

#define SENSOR_US_PERIOD_MS              200U
#define SENSOR_PRESS_PERIOD_MS           50U
#define SENSOR_STALE_MS                  2000U
#define SENSOR_PRESS_BASELINE_SAMPLES    16U

/* Fill these three values after running the independent pressure calibration
 * test. Zero values deliberately mean "not calibrated"; MPa must never be
 * fabricated from an arbitrary RAW count. */
#define SENSOR_PRESS_RAW_ZERO             0L
#define SENSOR_PRESS_RAW_REFERENCE        0L
#define SENSOR_PRESS_REFERENCE_KPA        0L
#define SENSOR_PRESS_MIN_CAL_SPAN          100L

/* Bench two-point conversion using the closest stable distance observed on
 * the installed ultrasonic module. 500 mm is empty; 230 mm is full. */
#define SENSOR_BENCH_EMPTY_DISTANCE_MM   500U
#define SENSOR_BENCH_FULL_DISTANCE_MM    230U

static SensorServiceData_t sensor_data;
static uint32_t sensor_next_us_ms;
static uint32_t sensor_next_press_ms;
static uint32_t sensor_last_us_driver_ms;
static uint32_t sensor_last_us_timeout_count;
static uint8_t sensor_us_failure_count;
static uint32_t sensor_pressure_baseline_samples;
static int32_t sensor_pressure_baseline_raw;
static bool sensor_pressure_baseline_ready;

static bool SensorService_PressureCalibrationValid(void)
{
    int32_t span = SENSOR_PRESS_RAW_REFERENCE - SENSOR_PRESS_RAW_ZERO;

    if (span < 0L)
    {
        span = -span;
    }
    return (SENSOR_PRESS_REFERENCE_KPA > 0L) &&
           (span >= SENSOR_PRESS_MIN_CAL_SPAN);
}

static int32_t SensorService_ScalePressure(int32_t raw_delta,
                                           int32_t reference_kpa,
                                           int32_t raw_span)
{
    int64_t scaled;

    if (raw_span == 0L)
    {
        return 0L;
    }
    scaled = (int64_t)raw_delta * (int64_t)reference_kpa;
    return (int32_t)(scaled / raw_span);
}

static int32_t SensorService_ConvertPressureKpa(int32_t raw)
{
    const int32_t raw_span =
        SENSOR_PRESS_RAW_REFERENCE - SENSOR_PRESS_RAW_ZERO;
    int32_t result;

    if (!SensorService_PressureCalibrationValid() || (raw_span == 0L))
    {
        return 0L;
    }
    result = SensorService_ScalePressure(raw - SENSOR_PRESS_RAW_ZERO,
                                         SENSOR_PRESS_REFERENCE_KPA,
                                         raw_span);
    return (result < 0L) ? 0L : result;
}

static bool SensorService_TimeReached(uint32_t now_ms, uint32_t target_ms)
{
    return ((int32_t)(now_ms - target_ms) >= 0);
}

static int32_t SensorService_Abs(int32_t value)
{
    return (value < 0) ? -value : value;
}

static uint8_t SensorService_DistanceToLevel(uint32_t distance_mm)
{
    uint32_t water_height_mm;

    if (distance_mm >= SENSOR_BENCH_EMPTY_DISTANCE_MM)
    {
        return 0U;
    }
    if (distance_mm <= SENSOR_BENCH_FULL_DISTANCE_MM)
    {
        return 100U;
    }
    water_height_mm = SENSOR_BENCH_EMPTY_DISTANCE_MM - distance_mm;
    return (uint8_t)((water_height_mm * 100U) /
                     (SENSOR_BENCH_EMPTY_DISTANCE_MM -
                      SENSOR_BENCH_FULL_DISTANCE_MM));
}

void SensorService_Init(uint32_t now_ms)
{
    HX710B_Init();
    Ultrasonic_Init();

    sensor_data.ultrasonic_valid = false;
    sensor_data.pressure_valid = false;
    sensor_data.pressure_calibrated =
        SensorService_PressureCalibrationValid();
    sensor_data.all_valid = false;
    sensor_data.distance_mm = 0U;
    sensor_data.level_percent = 0U;
    sensor_data.pressure_raw = 0;
    sensor_data.pressure_delta_raw = 0;
    sensor_data.pressure_kpa = 0;
    sensor_data.ultrasonic_last_update_ms = 0U;
    sensor_data.pressure_last_update_ms = 0U;
    sensor_next_us_ms = now_ms;
    sensor_next_press_ms = now_ms;
    sensor_last_us_driver_ms = 0U;
    sensor_last_us_timeout_count = 0U;
    sensor_us_failure_count = 0U;
    sensor_pressure_baseline_samples = 0U;
    sensor_pressure_baseline_raw = 0;
    sensor_pressure_baseline_ready = false;
}

void SensorService_Process(uint32_t now_ms)
{
    const Ultrasonic_Data_t *us;
    const HX710B_Data_t *pressure;
    int32_t raw;

    Ultrasonic_Update(now_ms);

    if (SensorService_TimeReached(now_ms, sensor_next_us_ms))
    {
        sensor_next_us_ms = now_ms + SENSOR_US_PERIOD_MS;
        (void)Ultrasonic_Start(now_ms);
    }

    if (SensorService_TimeReached(now_ms, sensor_next_press_ms))
    {
        sensor_next_press_ms = now_ms + SENSOR_PRESS_PERIOD_MS;
        if (!Ultrasonic_GetData()->busy &&
            (HX710B_TryRead(&raw) == HX710B_STATUS_OK))
        {
            HX710B_AddSample(raw);
            sensor_data.pressure_last_update_ms = now_ms;
            sensor_pressure_baseline_samples++;
            if (!sensor_pressure_baseline_ready &&
                (sensor_pressure_baseline_samples >=
                 SENSOR_PRESS_BASELINE_SAMPLES))
            {
                SensorService_CapturePressureBaseline();
            }
        }
    }

    us = Ultrasonic_GetData();
    if (us->last_update_ms != sensor_last_us_driver_ms)
    {
        sensor_last_us_driver_ms = us->last_update_ms;
        if (us->valid)
        {
            sensor_data.distance_mm = us->distance_mm;
            sensor_data.level_percent =
                SensorService_DistanceToLevel(us->distance_mm);
            sensor_data.ultrasonic_last_update_ms = now_ms;
            sensor_us_failure_count = 0U;
        }
        else if (us->timeout_count != sensor_last_us_timeout_count)
        {
            sensor_last_us_timeout_count = us->timeout_count;
            if (sensor_us_failure_count < 0xFFU)
            {
                sensor_us_failure_count++;
            }
        }
    }

    pressure = HX710B_GetData();
    sensor_data.pressure_raw = pressure->filtered_raw;
    if (sensor_data.pressure_calibrated && pressure->valid)
    {
        sensor_data.pressure_kpa =
            SensorService_ConvertPressureKpa(pressure->filtered_raw);
    }
    if (sensor_pressure_baseline_ready)
    {
        sensor_data.pressure_delta_raw =
            SensorService_Abs(pressure->filtered_raw -
                              sensor_pressure_baseline_raw);
    }

    sensor_data.ultrasonic_valid =
        (sensor_us_failure_count < 3U) &&
        (sensor_data.ultrasonic_last_update_ms != 0U) &&
        ((uint32_t)(now_ms - sensor_data.ultrasonic_last_update_ms) <=
         SENSOR_STALE_MS);
    sensor_data.pressure_valid = pressure->valid &&
        sensor_pressure_baseline_ready &&
        (sensor_data.pressure_last_update_ms != 0U) &&
        ((uint32_t)(now_ms - sensor_data.pressure_last_update_ms) <=
         SENSOR_STALE_MS);
    sensor_data.all_valid = sensor_data.ultrasonic_valid &&
                            sensor_data.pressure_valid;
}

const SensorServiceData_t *SensorService_GetData(void)
{
    return &sensor_data;
}

void SensorService_CapturePressureBaseline(void)
{
    const HX710B_Data_t *pressure = HX710B_GetData();

    if (pressure->valid)
    {
        sensor_pressure_baseline_raw = pressure->filtered_raw;
        sensor_pressure_baseline_ready = true;
        sensor_data.pressure_delta_raw = 0;
    }
}

bool SensorService_IsPressureCalibrated(void)
{
    return sensor_data.pressure_calibrated;
}

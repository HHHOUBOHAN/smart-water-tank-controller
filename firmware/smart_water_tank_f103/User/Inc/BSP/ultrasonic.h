#ifndef BSP_ULTRASONIC_H
#define BSP_ULTRASONIC_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool valid;
    bool busy;
    uint32_t distance_mm;
    uint32_t pulse_width_us;
    uint32_t last_update_ms;
    uint32_t timeout_count;
} Ultrasonic_Data_t;

void Ultrasonic_Init(void);
bool Ultrasonic_Start(uint32_t now_ms);
void Ultrasonic_Update(uint32_t now_ms);
void Ultrasonic_HandleExti(uint16_t gpio_pin, uint32_t now_ms);
const Ultrasonic_Data_t *Ultrasonic_GetData(void);

#endif /* BSP_ULTRASONIC_H */

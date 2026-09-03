#ifndef BSP_HX710B_H
#define BSP_HX710B_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    HX710B_STATUS_OK = 0,
    HX710B_STATUS_NOT_READY,
    HX710B_STATUS_INVALID_ARGUMENT
} HX710B_Status_t;

typedef struct
{
    bool valid;
    int32_t raw;
    int32_t filtered_raw;
    int32_t zero_offset;
    uint32_t sample_count;
} HX710B_Data_t;

void HX710B_Init(void);
bool HX710B_IsReady(void);
HX710B_Status_t HX710B_TryRead(int32_t *raw_value);
void HX710B_AddSample(int32_t raw_value);
void HX710B_Tare(void);
const HX710B_Data_t *HX710B_GetData(void);

#endif /* BSP_HX710B_H */

#pragma once
#include "./core.h"

typedef struct base_Time
{
    uint8_t monthOfYear; // 1-12
    uint8_t dayOfMonth; // 1-31
    uint8_t hourOfDay; // 0-23
} base_Time;
es_t base_time(base_Time* pTime);

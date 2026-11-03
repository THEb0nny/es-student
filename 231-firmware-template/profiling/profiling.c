#include "profiling.h"
#include <stddef.h>
#include "pico/stdlib.h"

#if PROFILING_ENABLED

typedef struct {
    const char *name;
    bool active;
    bool running;
    uint32_t count;
    uint32_t sum_us;
    uint32_t max_us;
    uint32_t start_us;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id >= PROFILING_MAX_STOPWATCHES)
    {
        return;
    }

    stopwatches[id].name = name;
    stopwatches[id].active = (name != NULL);
    stopwatches[id].running = false;
    stopwatches[id].count = 0;
    stopwatches[id].sum_us = 0;
    stopwatches[id].max_us = 0;
    stopwatches[id].start_us = 0;
}

void profiling_start(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES || !stopwatches[id].active)
    {
        return;
    }

    stopwatches[id].start_us = time_us_32();
    stopwatches[id].running = true;
}

void profiling_stop(uint32_t id)
{
    if (id >= PROFILING_MAX_STOPWATCHES || !stopwatches[id].active || !stopwatches[id].running)
    {
        return;
    }

    stopwatches[id].running = false;
    uint32_t duration_us = time_us_32() - stopwatches[id].start_us;
    stopwatches[id].count++;
    stopwatches[id].sum_us += duration_us;
    if (duration_us > stopwatches[id].max_us)
    {
        stopwatches[id].max_us = duration_us;
    }
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_MAX_STOPWATCHES || !stopwatches[id].active || result == NULL)
    {
        return false;
    }

    result->name = stopwatches[id].name;
    result->count = stopwatches[id].count;
    result->max_us = stopwatches[id].max_us;
    if (stopwatches[id].count > 0)
    {
        result->mean_us = (float)stopwatches[id].sum_us / (float)stopwatches[id].count;
    }
    else
    {
        result->mean_us = 0.0f;
    }

    return true;
}

void profiling_reset(void)
{
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        stopwatches[i].count = 0;
        stopwatches[i].sum_us = 0;
        stopwatches[i].max_us = 0;
    }
}

#endif

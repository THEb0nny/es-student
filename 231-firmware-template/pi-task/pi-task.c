#include "pi-task.h"
#include <stddef.h>
#include "systime/systime.h"

// прикидка: член ряда 4,7 мкс, 100 членов — около 0,47 мс, укладывается в 0,5 мс
#define PI_TERMS_PER_CALL 100

static pi_state_t pi_state = PI_STATE_NOT_STARTED;
static uint32_t pi_n = 0;
static uint32_t pi_k = 0;
static double pi_sum = 0.0;
static double pi_sign = 1.0;
static double pi_result = 0.0;
static uint64_t pi_start_us = 0;
static uint64_t pi_spent_ms = 0;

void pi_task_init(void)
{
    pi_state = PI_STATE_NOT_STARTED;
    pi_n = 0;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_result = 0.0;
    pi_start_us = 0;
    pi_spent_ms = 0;
}

bool pi_task_start(uint32_t terms)
{
    if (pi_state == PI_STATE_RUNNING)
    {
        return false;
    }

    pi_n = terms;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;
    pi_start_us = systime_us();
    pi_state = PI_STATE_RUNNING;

    return true;
}

pi_state_t pi_task_get_state(void)
{
    return pi_state;
}

bool pi_task_get_result(double *result, uint64_t *time_ms)
{
    if (pi_state != PI_STATE_READY)
    {
        return false;
    }

    if (result != NULL)
    {
        *result = pi_result;
    }
    if (time_ms != NULL)
    {
        *time_ms = pi_spent_ms;
    }

    return true;
}

void pi_task_handle(void)
{
    if (pi_state != PI_STATE_RUNNING)
    {
        return;
    }

    uint32_t end = pi_k + PI_TERMS_PER_CALL;
    if (end < pi_k || end > pi_n)
    {
        end = pi_n;
    }

    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * (double)pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k >= pi_n)
    {
        pi_result = 4.0 * pi_sum;
        pi_spent_ms = (systime_us() - pi_start_us) / 1000;
        pi_state = PI_STATE_READY;
    }
}

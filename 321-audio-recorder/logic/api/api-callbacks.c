#include "api-commands.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "firmware.h"
#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include "led-task/led-task.h"
#include "button-task/button-task.h"
#include "pi-task/pi-task.h"
#include "platform/platform.h"
#include "telemetry-task/telemetry-task.h"

static bool parse_u32(const char *text, uint32_t *value)
{
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

static void cmd_info(const command_t *command)
{
    (void)command;
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void cmd_uptime(const command_t *command)
{
    (void)command;
    printf("uptime: %llu ms\n", systime_ms());
}

static void print_led_state(void)
{
    const char *state_str = "unknown";
    switch (led_task_get_state())
    {
    case LED_STATE_OFF:
        state_str = "off";
        break;
    case LED_STATE_ON:
        state_str = "on";
        break;
    case LED_STATE_BLINK:
        state_str = "blink";
        break;
    }

    printf("led: %s, period %u ms\n", state_str, (unsigned)led_task_get_period_ms());
}

static void cmd_led_enable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_ON);
    print_led_state();
}

static void cmd_led_disable(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_OFF);
    print_led_state();
}

static void cmd_led_blink(const command_t *command)
{
    (void)command;
    led_task_set_state(LED_STATE_BLINK);
    print_led_state();
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (!led_task_set_period_ms(period_ms))
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    print_led_state();
}

static void cmd_button(const command_t *command)
{
    (void)command;
    const char *state_str = button_task_is_pressed() ? "pressed" : "released";
    printf("button: %s, presses %u\n", state_str, (unsigned)button_task_get_press_count());
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t terms = 1000000;

    if (command->argc > 1)
    {
        printf("error: usage pi_start [<terms>]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &terms))
        {
            printf("error: usage pi_start [<terms>]\n");
            return;
        }

        if (terms == 0)
        {
            printf("error: terms must be greater than 0\n");
            return;
        }
    }

    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }

    printf("pi: started, %u terms\n", (unsigned)terms);
}

static void cmd_pi(const command_t *command)
{
    (void)command;
    pi_state_t state = pi_task_get_state();

    switch (state)
    {
    case PI_STATE_NOT_STARTED:
        printf("pi: not started\n");
        break;

    case PI_STATE_RUNNING:
        printf("pi: running\n");
        break;

    case PI_STATE_READY:
    {
        double result;
        uint64_t time_ms;
        if (pi_task_get_result(&result, &time_ms))
        {
            printf("pi: %.9f in %llu ms\n", result, time_ms);
        }
        break;
    }
    }
}

static void cmd_profiling(const command_t *command)
{
    (void)command;
#if PROFILING_ENABLED
    profiling_result_t result;
    for (uint32_t id = 0; id < PROFILING_MAX_STOPWATCHES; id++)
    {
        if (profiling_get(id, &result))
        {
            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   result.name, (unsigned)result.count, result.mean_us, (unsigned)result.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *command)
{
    (void)command;
    profiling_reset();
    printf("profiling reset\n");
}

static void cmd_get_voltage(const command_t *command)
{
    (void)command;
    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    printf("voltage: %.3f V\n", platform_voltage_read());
}

static void cmd_get_temp(const command_t *command)
{
    (void)command;
    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    printf("temp: %.1f C\n", platform_temp_read());
}

#define TELEMETRY_DEFAULT_PERIOD_MS 100

void api_print_telemetry(const telemetry_record_t *record)
{
    if (record == NULL)
    {
        return;
    }
    printf("tm: %llu %.3f %.1f\n", record->time_us, record->voltage_v, record->temp_c);
}

static void cmd_tm_start(const command_t *command)
{
    uint32_t period_ms = TELEMETRY_DEFAULT_PERIOD_MS;

    if (command->argc > 1)
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    if (command->argc == 1)
    {
        if (!parse_u32(command->argv[0], &period_ms))
        {
            printf("error: usage tm_start [<period_ms>]\n");
            return;
        }

        if (period_ms < 10)
        {
            printf("error: period_ms must be at least 10\n");
            return;
        }
    }

    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }

    if (!telemetry_task_start(period_ms))
    {
        printf("error: period_ms must be at least 10\n");
        return;
    }

    printf("telemetry: started, period %u ms\n", (unsigned)period_ms);
}

static void cmd_tm_stop(const command_t *command)
{
    (void)command;
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

static void cmd_timer_blink_start(const command_t *command)
{
    uint32_t period_ms;

    if (command->argc != 1 || !parse_u32(command->argv[0], &period_ms))
    {
        printf("error: usage timer_blink_start <period_ms>\n");
        return;
    }

    if (!platform_blink_start(period_ms))
    {
        printf("error: period_ms must be 10..60000\n");
        return;
    }

    led_task_set_state(LED_STATE_OFF);
    printf("timer_blink: started, period %u ms\n", (unsigned)period_ms);
}

static void cmd_timer_blink_stop(const command_t *command)
{
    (void)command;
    uint32_t toggles = platform_blink_stop();
    printf("timer_blink: stopped, %u toggles\n", (unsigned)toggles);
}

static void cmd_cap_start(const command_t *command)
{
    uint32_t count;
    uint32_t period_us;

    if (command->argc != 2 || !parse_u32(command->argv[0], &count) || !parse_u32(command->argv[1], &period_us))
    {
        printf("error: usage cap_start <N> <period_us>\n");
        return;
    }

    if (count == 0 || count > PLATFORM_CAPTURE_MAX || period_us < 10)
    {
        printf("error: N must be 1..2000, period_us at least 10\n");
        return;
    }

    if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || !platform_capture_start(count, period_us))
    {
        printf("error: adc busy\n");
        return;
    }

    printf("capture: started, %u samples, period %u us\n", (unsigned)count, (unsigned)period_us);
}

static void cmd_cap_dump(const command_t *command)
{
    (void)command;
    if (!platform_capture_done())
    {
        printf("error: no capture\n");
        return;
    }

    platform_capture_t capture;
    platform_capture_get(&capture);

    printf("capture: %u samples, period %u us, interval %u..%u us, handler %.1f %%\n",
           (unsigned)capture.count, (unsigned)capture.period_us,
           (unsigned)capture.min_interval_us, (unsigned)capture.max_interval_us,
           capture.handler_percent);

    for (uint32_t i = 0; i < capture.count; i++)
    {
        if (i % 16 == 0)
        {
            printf("cap:");
        }
        printf(" %u", (unsigned)capture.samples[i]);
        if (i % 16 == 15 || i + 1 == capture.count)
        {
            printf("\n");
        }
    }
}

const api_command_t api_commands[] = {
    { "info", "device passport", cmd_info },
    { "uptime", "time since reset", cmd_uptime },
    { "led_enable", "turn led on", cmd_led_enable },
    { "led_disable", "turn led off", cmd_led_disable },
    { "led_blink", "turn led blink", cmd_led_blink },
    { "led_period", "set blink period: led_period <period_ms>", cmd_led_period },
    { "button", "button state and press count", cmd_button },
    { "pi_start", "start pi calculation: pi_start [<terms>]", cmd_pi_start },
    { "pi", "pi calculation result or status", cmd_pi },
    { "profiling", "stopwatch profiling: profiling", cmd_profiling },
    { "profiling_reset", "reset profiling stopwatches", cmd_profiling_reset },
    { "get_voltage", "voltage on GP26, ADC input 0", cmd_get_voltage },
    { "get_temp", "chip temperature, degrees Celsius", cmd_get_temp },
    { "tm_start", "start telemetry: tm_start [<period_ms>]", cmd_tm_start },
    { "tm_stop", "stop telemetry", cmd_tm_stop },
    { "timer_blink_start", "blink led by timer: timer_blink_start <period_ms>", cmd_timer_blink_start },
    { "timer_blink_stop", "stop timer blink, number of toggles", cmd_timer_blink_stop },
    { "cap_start", "capture samples: cap_start <N> <period_us>", cmd_cap_start },
    { "cap_dump", "print captured samples", cmd_cap_dump }
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);

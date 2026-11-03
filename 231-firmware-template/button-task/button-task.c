#include "button-task.h"
#include "pico/stdlib.h"
#include "systime/systime.h"
#include "led-task.h"

const uint BUTTON_PIN = 15;
const uint32_t BUTTON_DEBOUNCE_MS = 20;
const uint32_t BUTTON_DEBOUNCE_US = 20000;

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t state_change_us = 0;
static uint32_t press_count = 0;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    button_state = BUTTON_STATE_RELEASED;
    state_change_us = 0;
    press_count = 0;
}

bool button_task_is_pressed(void)
{
    return button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}

void button_task_handle(void)
{
    bool pin_pressed = !gpio_get(BUTTON_PIN);
    uint64_t now_us = systime_us();

    switch (button_state)
    {
    case BUTTON_STATE_RELEASED:
        if (pin_pressed)
        {
            state_change_us = now_us;
            button_state = BUTTON_STATE_PRESSING;
        }
        break;

    case BUTTON_STATE_PRESSING:
        if (!pin_pressed)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - state_change_us >= BUTTON_DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_PRESSED;
            press_count++;
            led_task_next_state();
        }
        break;

    case BUTTON_STATE_PRESSED:
        if (!pin_pressed)
        {
            state_change_us = now_us;
            button_state = BUTTON_STATE_RELEASING;
        }
        break;

    case BUTTON_STATE_RELEASING:
        if (pin_pressed)
        {
            button_state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - state_change_us >= BUTTON_DEBOUNCE_US)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}

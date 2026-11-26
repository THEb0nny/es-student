#include "button-task/button-task.h"
#include <stddef.h>
#include "platform/platform.h"
#include "systime/systime.h"

static const uint32_t BUTTON_DEBOUNCE_US = 20000;

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

static button_task_callback_t button_callback = NULL;
static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint64_t state_change_us = 0;
static uint32_t press_count = 0;

void button_task_init(button_task_callback_t on_press)
{
    button_callback = on_press;
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
    bool pin_pressed = platform_button_read();
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
            if (button_callback != NULL)
            {
                button_callback();
            }
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

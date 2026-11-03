#include "button-task/button-task.h"
#include <stddef.h>
#include "systime/systime.h"

static const uint32_t BUTTON_DEBOUNCE_US = 20000;

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

typedef struct {
    button_task_callback_t callback;
    button_state_t state;
    uint64_t state_change_us;
    uint32_t press_count;
} button_instance_t;

static button_instance_t buttons[PLATFORM_BUTTON_COUNT];

void button_task_init(platform_button_t button, button_task_callback_t on_press)
{
    if (button < PLATFORM_BUTTON_COUNT)
    {
        buttons[button].callback = on_press;
        buttons[button].state = BUTTON_STATE_RELEASED;
        buttons[button].state_change_us = 0;
        buttons[button].press_count = 0;
    }
}

bool button_task_is_pressed(platform_button_t button)
{
    if (button >= PLATFORM_BUTTON_COUNT)
    {
        return false;
    }
    return buttons[button].state == BUTTON_STATE_PRESSED ||
           buttons[button].state == BUTTON_STATE_RELEASING;
}

uint32_t button_task_get_press_count(platform_button_t button)
{
    if (button >= PLATFORM_BUTTON_COUNT)
    {
        return 0;
    }
    return buttons[button].press_count;
}

void button_task_handle(void)
{
    uint64_t now_us = systime_us();

    for (int i = 0; i < PLATFORM_BUTTON_COUNT; i++)
    {
        bool pin_pressed = platform_button_read((platform_button_t)i);

        switch (buttons[i].state)
        {
        case BUTTON_STATE_RELEASED:
            if (pin_pressed)
            {
                buttons[i].state_change_us = now_us;
                buttons[i].state = BUTTON_STATE_PRESSING;
            }
            break;

        case BUTTON_STATE_PRESSING:
            if (!pin_pressed)
            {
                buttons[i].state = BUTTON_STATE_RELEASED;
            }
            else if (now_us - buttons[i].state_change_us >= BUTTON_DEBOUNCE_US)
            {
                buttons[i].state = BUTTON_STATE_PRESSED;
                buttons[i].press_count++;
                if (buttons[i].callback != NULL)
                {
                    buttons[i].callback();
                }
            }
            break;

        case BUTTON_STATE_PRESSED:
            if (!pin_pressed)
            {
                buttons[i].state_change_us = now_us;
                buttons[i].state = BUTTON_STATE_RELEASING;
            }
            break;

        case BUTTON_STATE_RELEASING:
            if (pin_pressed)
            {
                buttons[i].state = BUTTON_STATE_PRESSED;
            }
            else if (now_us - buttons[i].state_change_us >= BUTTON_DEBOUNCE_US)
            {
                buttons[i].state = BUTTON_STATE_RELEASED;
            }
            break;
        }
    }
}

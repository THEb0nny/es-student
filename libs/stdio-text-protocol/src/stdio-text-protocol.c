#include "stdio-text-protocol/stdio-text-protocol.h"
#include <stdio.h>
#include "pico/stdlib.h"

static char line_buf[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_len = 0;
static bool is_truncated = false;
static command_t current_command;

void stdio_text_protocol_init(void)
{
    line_len = 0;
    is_truncated = false;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (line_len > 0)
        {
            line_len--;
            is_truncated = false;
            printf("\b \b");
        }
        return NULL;
    }

    if (symbol == '\r' || symbol == '\n')
    {
        if (line_len == 0)
        {
            return NULL;
        }

        printf("\r\n");
        line_buf[line_len] = '\0';

        current_command.name = NULL;
        current_command.argc = 0;
        current_command.truncated = is_truncated;

        bool in_word = false;
        for (uint32_t i = 0; i < line_len; i++)
        {
            if (line_buf[i] == ' ')
            {
                line_buf[i] = '\0';
                in_word = false;
            }
            else
            {
                if (!in_word)
                {
                    in_word = true;
                    if (current_command.name == NULL)
                    {
                        current_command.name = &line_buf[i];
                    }
                    else if (current_command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
                    {
                        current_command.argv[current_command.argc] = &line_buf[i];
                        current_command.argc++;
                    }
                    else
                    {
                        current_command.truncated = true;
                    }
                }
            }
        }

        line_len = 0;
        is_truncated = false;

        if (current_command.name == NULL)
        {
            return NULL;
        }

        return &current_command;
    }

    if (symbol >= 0x20 && symbol <= 0x7e)
    {
        if (line_len < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
        {
            line_buf[line_len++] = (char)symbol;
            putchar(symbol);
        }
        else
        {
            is_truncated = true;
        }
    }

    return NULL;
}

#include "log.h"

// Печатает префикс перед основным текстом сообщения (без переноса строки!)
void log_prefix(const char *level, const char *function, int line)
{
    printf("%s %s:%d ", level, function, line);
}

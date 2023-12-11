#include <stdarg.h>

void
PrintError(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    printf("\n");
    va_end(ap);
}

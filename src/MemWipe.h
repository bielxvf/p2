/*
 * Author(s):  Biel Sala , bielsalamimo@gmail.com
 */
#include <string.h>

void
MemWipe(void *p, int len)
{
    memset(p, 0, len);
}

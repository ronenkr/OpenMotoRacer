#ifndef FTOL_H
#define FTOL_H
#include <stdint.h>

/* 0x4b3048, the C library's float to int: truncated through fistp into 64 bits, the low half
   kept; past 2^63, and a NaN, the x87's 0x8000000000000000, whose low half is 0. C leaves all
   that undefined, and x86 and ARM answer differently. (double to 64 bits is slow on the
   MiSTer's ARM: the usual case first.) */
static inline int ftol(double x)
{
#ifdef __arm__
    /* (the ARM's conversion stops at the ends and makes a NaN 0; comparing doubles stalls it) */
    int r = (int)x;
    if (r != 2147483647 && r != -2147483647 - 1)
        return r;
#endif
    if (x > -2147483649.0 && x < 2147483648.0)
        return (int)x;
    if (x >= -9223372036854775808.0 && x < 9223372036854775808.0)
        return (int)(uint32_t)(uint64_t)(int64_t)x;
    return 0;
}

/* the same of a float (exact as a double): the FPU's own truncation when it is in range (the
   ESP32's doubles are done in software) */
static inline int ftolf(float x)
{
    if (x > -2147483648.0f && x < 2147483648.0f)
        return (int)x;
    return ftol((double)x);
}

#endif

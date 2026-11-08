/* The ESP32 (OMR_FAST_FLOAT, NOTES §4.26): what is seen and heard in single precision, the
   FPU's (the board's doubles are done in software). Included after a file's other headers, so
   their declarations keep their doubles; with -fsingle-precision-constant. Not for the riding
   (bikephys.c, cpu.c have their own OMR_FAST_PHYS). */
#ifndef FASTFLOAT_H
#define FASTFLOAT_H
/* a double that stays one: a function other files call keeps its type */
typedef double omr_double;
#endif

#ifdef OMR_FAST_FLOAT
#include <math.h>
#define double float
#define sqrt sqrtf
#define sin sinf
#define cos cosf
#define tan tanf
#define atan atanf
#define atan2 atan2f
#define asin asinf
#define acos acosf
#define fabs fabsf
#define floor floorf
#define ceil ceilf
#define pow powf
#define fmod fmodf
#define exp expf
#define log logf
#define ftol ftolf
#endif

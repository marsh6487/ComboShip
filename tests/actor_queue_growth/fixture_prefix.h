/* Native test dependencies. Queue, reset, mask and contact code comes from production. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef float f32;

#define SQ(x) ((x) * (x))
#define IS_ZERO(x) (fabsf(x) < 0.008f)
#define MASS_IMMOVABLE 255
#define MASS_HEAVY 254
enum { MASSTYPE_IMMOVABLE, MASSTYPE_HEAVY, MASSTYPE_NORMAL };

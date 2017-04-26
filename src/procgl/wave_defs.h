float sin1(float x);

#define PG_WAVE_SINE(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0x1, \
        .func1 = sin1, .scale = 1, .frequency = { 1 }, \
        __VA_ARGS__ } )

#include "ext/noise1234.h"
#define PG_WAVE_PERLIN(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0xF, \
        .func1 = perlin1, .func2 = perlin2, .func3 = perlin3, .func4 = perlin4, \
        .scale = 1, .frequency = { 1, 1, 1, 1 }, \
        __VA_ARGS__ } )


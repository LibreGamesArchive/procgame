#include <stdarg.h>

struct pg_wave {
    float phase[4], frequency[4], scale, add;
    enum {
        PG_WAVE_END = 0,
        PG_WAVE_CONSTANT,
        PG_WAVE_TRANSFORM,
        PG_WAVE_FUNCTION,
        PG_WAVE_ARRAY,
        PG_WAVE_MIX_FUNC,
        PG_WAVE_MODIFIER_EXPAND,
        PG_WAVE_MODIFIER_OCTAVES,
        PG_WAVE_MODIFIER_SEAMLESS_1D,
        PG_WAVE_MODIFIER_SEAMLESS_2D,
        PG_WAVE_MODIFIER_DISTORT } type;
    union {
        float constant;
        struct {
            unsigned dimension_mask;
            float (*func1)(float);
            float (*func2)(float, float);
            float (*func3)(float, float, float);
            float (*func4)(float, float, float, float);
        };
        struct { struct pg_wave* arr; unsigned len; };
        struct {
            enum { PG_WAVE_EXPAND_ADD, PG_WAVE_EXPAND_SUB, PG_WAVE_EXPAND_MUL,
                   PG_WAVE_EXPAND_DIV, PG_WAVE_EXPAND_AVG } op;
            enum { PG_WAVE_EXPAND_BEFORE, PG_WAVE_EXPAND_AFTER } mode;
        };
        struct { int octaves; float ratio; float decay; };
        struct { vec4 dist_v; void (*distort)(vec4 out, vec4 in, vec4 dist_v); };
        struct { float (*mix)(float a, float b, float k); float mix_k; };
    };
};

float pg_wave_sample(struct pg_wave* wave, int d, vec4 p);

/*  Super-hero macros   */
/*  See readme_wave.md  */
#define PG_WAVE_CONSTANT(c, ...) \
    ((struct pg_wave){ .type = PG_WAVE_CONSTANT, .constant = c, \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_TRANSFORM(...) \
    ((struct pg_wave){ .type = PG_WAVE_TRANSFORM, \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_ARRAY(w, l, ...) \
    ((struct pg_wave){ .type = PG_WAVE_ARRAY, .arr = (w), .len = (l), \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MIX(f, k, ...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, .mix = f, .mix_k = k, __VA_ARGS })
#define PG_WAVE_END()     ((struct pg_wave){ .type = PG_WAVE_END })

#define PG_WAVE_MOD_EXPAND(o, m, ...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER_EXPAND, .op = o, .mode = m, \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MOD_OCTAVES(...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER_OCTAVES, \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MOD_SEAMLESS_1D(...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER_SEAMLESS_1D, \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MOD_SEAMLESS_2D(...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER_SEAMLESS_2D, \
        .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MOD_DISTORT(d, ...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER_DISTORT, .distort = (d), __VA_ARGS__ })

/*  Built-in mix functions  */
float pg_wave_mix_min(float a, float b, float k);
float pg_wave_mix_min_abs(float a, float b, float k);
float pg_wave_mix_smin(float a, float b, float k);
float pg_wave_mix_smin_abs(float a, float b, float k);
float pg_wave_mix_max(float a, float b, float k);
#define PG_WAVE_MIX_MIN(...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, \
                       .mix = pg_wave_mix_min, \
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MIX_MIN_ABS(...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, \
                       .mix = pg_wave_mix_min_abs, \
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MIX_SMIN(...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, \
                       .mix = pg_wave_mix_smin, \
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MIX_SMIN_ABS(k, ...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, .mix_k = k \
                       .mix = pg_wave_mix_smin_abs,\
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MIX_MAX(...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, \
                       .mix = pg_wave_mix_max, \
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })
#define PG_WAVE_MIX_MAX_ABS(...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, \
                       .mix = pg_wave_mix_max_abs,\
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })

float pg_wave_mix_lerp(float a, float b, float k);
#define PG_WAVE_MIX_LERP(...) \
    ((struct pg_wave){ .type = PG_WAVE_MIX_FUNC, \
                       .mix = pg_wave_mix_lerp, .mix_k = 0.5f,\
                       .frequency = { 1, 1, 1, 1 }, .scale = 1, __VA_ARGS__ })

/*  Built-in primitive functions    */
#include "ext/noise1234.h"
#define PG_WAVE_FUNC_PERLIN(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0xF, \
        .func1 = perlin1, .func2 = perlin2, .func3 = perlin3, .func4 = perlin4, \
        .scale = 1, .frequency = { 1, 1, 1, 1 }, \
        __VA_ARGS__ } )

/*  The 1D functions (use PG_WAVE_MOD_EXPAND to get more out of these)   */
float pg_wave_sin1(float x);
#define PG_WAVE_FUNC_SINE(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0x1, \
        .func1 = pg_wave_sin1, .scale = 1, .frequency = { 1 }, \
        __VA_ARGS__ } )
float pg_wave_tri1(float x);
#define PG_WAVE_FUNC_TRIANGLE(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0x1, \
        .func1 = pg_wave_tri1, .scale = 1, .frequency = { 1 }, \
        __VA_ARGS__ } )
float pg_wave_square1(float x);
#define PG_WAVE_FUNC_SQUARE(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0x1, \
        .func1 = pg_wave_square1, .scale = 1, .frequency = { 1 }, \
        __VA_ARGS__ } )
float pg_wave_saw1(float x);
#define PG_WAVE_FUNC_SAW(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0x1, \
        .func1 = pg_wave_saw1, .scale = 1, .frequency = { 1 }, \
        __VA_ARGS__ } )

/*  Some basic math functions for utility   */
float pg_wave_dist1(float x);
float pg_wave_dist2(float x, float y);
float pg_wave_dist3(float x, float y, float z);
float pg_wave_dist4(float x, float y, float z, float w);
#define PG_WAVE_FUNC_DISTANCE(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0xF, \
        .func1 = pg_wave_dist1, .func2 = pg_wave_dist2, \
        .func3 = pg_wave_dist3, .func4 = pg_wave_dist4, \
        .scale = 1, .frequency = { 1, 1, 1, 1 }, \
        __VA_ARGS__ } )

float pg_wave_max1(float x);
float pg_wave_max2(float x, float y);
float pg_wave_max3(float x, float y, float z);
float pg_wave_max4(float x, float y, float z, float w);
#define PG_WAVE_FUNC_MAX(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0xF, \
        .func1 = pg_wave_max1, .func2 = pg_wave_max2, \
        .func3 = pg_wave_max3, .func4 = pg_wave_max4, \
        .scale = 1, .frequency = { 1, 1, 1, 1 }, \
        __VA_ARGS__ } )

float pg_wave_min1(float x);
float pg_wave_min2(float x, float y);
float pg_wave_min3(float x, float y, float z);
float pg_wave_min4(float x, float y, float z, float w);
#define PG_WAVE_FUNC_MIN(...) \
    ( (struct pg_wave) { \
        .type = PG_WAVE_FUNCTION, .dimension_mask = 0xF, \
        .func1 = pg_wave_min1, .func2 = pg_wave_min2, \
        .func3 = pg_wave_min3, .func4 = pg_wave_min4, \
        .scale = 1, .frequency = { 1, 1, 1, 1 }, \
        __VA_ARGS__ } )


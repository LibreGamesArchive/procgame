#include <stdarg.h>

/*  Built-in wave definitions are in wave_defs.h    */

struct pg_wave {
    enum {
        PG_WAVE_CONSTANT,
        PG_WAVE_FUNCTION,
        PG_WAVE_ARRAY,
        PG_WAVE_MODIFIER } type;
    union {
        float constant;
        struct {
            unsigned dimension_mask;
            float (*func1)(float);
            float (*func2)(float, float);
            float (*func3)(float, float, float);
            float (*func4)(float, float, float, float);
            float frequency[4];
            float phase[4];
            float scale;
            float add;
        };
        struct {
            struct pg_wave* arr;
            unsigned len;
        };
        struct {
            enum {
                PG_WAVE_MOD_EXPAND,
                PG_WAVE_MOD_OCTAVES,
                PG_WAVE_MOD_SEAMLESS_1D,
                PG_WAVE_MOD_SEAMLESS_2D,
                PG_WAVE_MOD_MIX_FUNC,
                PG_WAVE_MOD_DISTORT
            } mod;
            struct {
                enum {
                    PG_WAVE_EXPAND_ADD,
                    PG_WAVE_EXPAND_SUB,
                    PG_WAVE_EXPAND_MUL,
                    PG_WAVE_EXPAND_DIV,
                    PG_WAVE_EXPAND_AVG
                } op;
                enum {
                    PG_WAVE_EXPAND_BEFORE,
                    PG_WAVE_EXPAND_AFTER,
                } mode;
            };
            union {
                struct {
                    int octaves;
                    float ratio;
                    float decay;
                };
                struct {
                    vec4 dist_v;
                    void (*distort)(vec4 out, vec4 in, vec4 dist_v);
                };
                float (*mix)(float a, float b);
            };
        };
    };
};

#define PG_WAVE_ARRAY(w, l) \
    ((struct pg_wave){ .type = PG_WAVE_ARRAY, .arr = (w), .len = (l) })
#define PG_WAVE_MODIFY(mod, ...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = __VA_ARGS__ }
#define PG_WAVE_EXPAND(...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_EXPAND, \
                       __VA_ARGS__  })
#define PG_WAVE_OCTAVES(...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_OCTAVES, \
                       __VA_ARGS__ })
#define PG_WAVE_SEAMLESS_1D() \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_SEAMLESS_1D })
#define PG_WAVE_SEAMLESS_2D() \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_SEAMLESS_2D })
#define PG_WAVE_MIX_FUNC(...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_MIX_FUNC, \
                       __VA_ARGS__ })
#define PG_WAVE_DISTORT(d, ...) \
    ((struct pg_wave){ .type = PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_DISTORT, \
                       __VA_ARGS__ })

float pg_wave_sample(struct pg_wave* wave, int d, vec4 p);

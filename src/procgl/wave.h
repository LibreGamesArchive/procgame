/*  A wave handling API
    -------------------
    A struct pg_wave can be in one of two states: a wave function, or waveform data.
    As a wave function, its `func` member points to a function taking a float
    and returning a float. As waveform data, its `samples` member will be
    `num_samples` elements long.
    The interface to wave functions and waveform data is identical. Both can be
    repeated at any given frequency, used as samples for other waves, or
    encoded to ints, in exactly the same way. To copy waveform data sample-for-
    sample, then, only provide a frequency of 1.  */

#include <stdarg.h>

struct pg_wave {
    int dim;
    float frequency[4];
    float phase[4];
    float scale;
    float add;
    enum { WAVE_CONSTANT, WAVE_FUNCTION, WAVE_FUNCTION_STATE, WAVE_COMPOSITE } type;
    union {
        struct {
            float (*func1)(float);
            float (*func2)(float, float);
            float (*func3)(float, float, float);
            float (*func4)(float, float, float, float);
        };
        struct {
            void* state;
            float (*func1_s)(void*, float);
            float (*func2_s)(void*, float, float);
            float (*func3_s)(void*, float, float, float);
            float (*func4_s)(void*, float, float, float, float);
        };
        struct {
            struct pg_wave* comp0;
            struct pg_wave* comp1;
            float influence;
            float (*mix)(float,float,float);
        };
    };
};

float pg_wave_sample(struct pg_wave* wave, int d, ...);

void pg_wave_composite(struct pg_wave* wave,
                    struct pg_wave* comp0, struct pg_wave* comp1,
                    float influence, float (*mix)(float, float, float));
void pg_wave_init_sine(struct pg_wave* wave);
void pg_wave_init_crater(struct pg_wave* wave);
void pg_wave_crater_add(struct pg_wave* wave, float x, float y, float r);
void pg_wave_deinit(struct pg_wave* wave);

float pg_wave_mix_add(float a, float b, float x);

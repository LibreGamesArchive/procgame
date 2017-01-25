#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "wave.h"

float procgl_wave_sample(struct procgl_wave* wave, int d, ...)
{
    va_list args;
    va_start(args, d);
    float _x = (d > 0) ?
        (va_arg(args, double) + wave->phase[0]) * wave->frequency[0]
        : (wave->dim > 0) ? wave->phase[0] * wave->frequency[0] : 0;
    float _y = (d > 1) ?
        (va_arg(args, double) + wave->phase[1]) * wave->frequency[1]
        : (wave->dim > 1) ? wave->phase[1] * wave->frequency[1] : 0;
    float _z = (d > 2) ?
        (va_arg(args, double) + wave->phase[2]) * wave->frequency[2]
        : (wave->dim > 2) ? wave->phase[2] * wave->frequency[2] : 0;
    float _w = (d > 3) ?
        (va_arg(args, double) + wave->phase[3]) * wave->frequency[3]
        : (wave->dim > 3) ? wave->phase[3] * wave->frequency[3] : 0;
    va_end(args);
    int dim = (wave->dim >= 0) ? wave->dim : d;
    switch(wave->type) {
    case WAVE_CONSTANT: return wave->add * wave->scale;
    case WAVE_FUNCTION:
        switch(dim) {
        case 0: return wave->add * wave->scale;
        case 1: return (!wave->func1) ? 0 :
            (wave->func1(_x) + wave->add) * wave->scale;
        case 2: return (!wave->func2) ? 0 :
            (wave->func2(_x, _y) + wave->add) * wave->scale;
        case 3: return (!wave->func3) ? 0 :
            (wave->func3(_x, _y, _z) + wave->add) * wave->scale;
        case 4: return (!wave->func4) ? 0 :
            (wave->func4(_x, _y, _z, _w) + wave->add) * wave->scale;
        }
    case WAVE_FUNCTION_STATE:
        switch(dim) {
        case 0: return wave->add * wave->scale;
        case 1: return (!wave->func1_s) ? 0 :
            (wave->func1_s(wave->state, _x) + wave->add) * wave->scale;
        case 2: return (!wave->func2_s) ? 0 :
            (wave->func2_s(wave->state, _x, _y) + wave->add) * wave->scale;
        case 3: return (!wave->func3_s) ? 0 :
            (wave->func3_s(wave->state, _x, _y, _z) + wave->add) * wave->scale;
        case 4: return (!wave->func4_s) ? 0 :
            (wave->func4_s(wave->state, _x, _y, _z, _w) + wave->add) * wave->scale;
        }
    case WAVE_COMPOSITE: {
        float w0, w1;
        switch(dim) {
        case 0:
            w0 = procgl_wave_sample(wave->comp0, 0);
            w1 = procgl_wave_sample(wave->comp1, 0);
            break;
        case 1:
            w0 = procgl_wave_sample(wave->comp0, 1, _x);
            w1 = procgl_wave_sample(wave->comp1, 1, _x);
            break;
        case 2:
            w0 = procgl_wave_sample(wave->comp0, 2, _x, _y);
            w1 = procgl_wave_sample(wave->comp1, 2, _x, _y);
            break;
        case 3:
            w0 = procgl_wave_sample(wave->comp0, 3, _x, _y, _z);
            w1 = procgl_wave_sample(wave->comp1, 3, _x, _y, _z);
            break;
        case 4:
            w0 = procgl_wave_sample(wave->comp0, 4, _x, _y, _z, _w);
            w1 = procgl_wave_sample(wave->comp1, 4, _x, _y, _z, _w);
        }
        return wave->mix(w0, w1, wave->influence);
        }
    }
    return 0;
}

void procgl_wave_composite(struct procgl_wave* wave,
                    struct procgl_wave* comp0, struct procgl_wave* comp1,
                    float influence, float (*mix)(float, float, float))
{
    
    *wave = (struct procgl_wave) {
        .dim = -1, .type = WAVE_COMPOSITE,
        .frequency = { 1, 1, 1, 1 }, .scale = 1,
        .comp0 = comp0, .comp1 = comp1, .influence = influence, .mix = mix
    };
}

static float sin1(float x)
{
    return sin(x);
}
static float sin2(float x, float y)
{
    return (sin(x) + sin(y)) / 2;
}
static float sin3(float x, float y, float z)
{
    return (sin(x) + sin(y) + sin(z)) / 3;
}
static float sin4(float x, float y, float z, float w)
{
    return (sin(x) + sin(y) + sin(z) + sin(w)) / 4;
}

void procgl_wave_init_sine(struct procgl_wave* wave)
{
    *wave = (struct procgl_wave) {
        .dim = -1, .type = WAVE_FUNCTION,
        .frequency = { 1, 1, 1, 1 }, .scale = 1,
        .func1 = sin1, .func2 = sin2, .func3 = sin3, .func4 = sin4
    };
}

float procgl_wave_mix_add(float a, float b, float x)
{
    return a + b;
}

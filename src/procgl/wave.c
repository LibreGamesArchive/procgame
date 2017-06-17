#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "ext/linmath.h"
#include "wave.h"

static float pg_wave_sample_array(struct pg_wave* wave, unsigned n,
                                  unsigned d, vec4 p)
{
    unsigned d_ = d;
    vec4 p_ = {
        ((d < 0) ? 0 : p[0]),
        ((d < 1) ? 0 : p[1]),
        ((d < 2) ? 0 : p[2]),
        ((d < 3) ? 0 : p[3]) };
    float s = 0;
    int i;
    struct pg_wave* iter = wave;
    for(i = 0; i < n; ++i, ++iter) {
        switch(iter->type) {
        case PG_WAVE_CONSTANT:
            s += wave->constant;
            break;
        case PG_WAVE_ARRAY:
            s += pg_wave_sample_array(iter->arr, iter->len, d_, p_);
            break;
        case PG_WAVE_FUNCTION:
            if(!(iter->dimension_mask & (1 << (d_ - 1)))) return 0;
            s += pg_wave_sample(iter, d_, p_);
            break;
        case PG_WAVE_MODIFIER:
            switch(iter->mod) {
            case PG_WAVE_MOD_OCTAVES: {
                int j;
                float f = 1, v = 1;
                for(j = 0; j < iter->octaves; ++j) {
                    s += v * pg_wave_sample_array(iter + 1, n - i - 1, d_,
                        (vec4){ p_[0] * f, p_[1] * f, p_[2] * f, p_[3] * f });
                    f *= iter->ratio;
                    v *= iter->decay;
                }
                i = n;
                break;
            } case PG_WAVE_MOD_SEAMLESS_1D: {
                d_ = 2;
                vec4_set(p_, cos(p_[0] * 2 * M_PI), sin(p_[0] * 2 * M_PI),
                         0, 0);
                break;
            } case PG_WAVE_MOD_SEAMLESS_2D: {
                d_ = 4;
                vec4_set(p_, cos(p_[0] * 2 * M_PI), cos(p_[1] * 2 * M_PI),
                             sin(p_[0] * 2 * M_PI), sin(p_[1] * 2 * M_PI));
                break;
            } case PG_WAVE_MOD_MIX_FUNC: {
                float s_ = pg_wave_sample_array(iter + 1, n - i - 1, d_, p_);
                s = iter->mix(s, s_);
                i = n;
                break;
            } case PG_WAVE_MOD_DISTORT: {
                iter->distort(p_, p_, iter->dist_v);
                break;
            } case PG_WAVE_MOD_EXPAND: {
                if(d_ == 1) continue;
                switch(iter->mode) {
                case PG_WAVE_EXPAND_BEFORE: {
                    int j;
                    float e = p_[0];
                    switch(iter->op) {
                        case PG_WAVE_EXPAND_ADD: case PG_WAVE_EXPAND_AVG:
                            for(j = 1; j < d_; ++j) e += p_[j];
                            break;
                        case PG_WAVE_EXPAND_SUB:
                            for(j = 1; j < d_; ++j) e -= p_[j];
                            break;
                        case PG_WAVE_EXPAND_MUL:
                            for(j = 1; j < d_; ++j) e *= p_[j];
                            break;
                        case PG_WAVE_EXPAND_DIV:
                            for(j = 1; j < d_; ++j) e /= p_[j];
                            break;
                    }
                    if(iter->op == PG_WAVE_EXPAND_AVG) e /= d_;
                    s += pg_wave_sample_array(iter + 1, n - i - 1, 1,
                            (vec4){ e });
                    break;
                } case PG_WAVE_EXPAND_AFTER: {
                    float e[d_];
                    int j;
                    for(j = 0; j < d_; ++j) {
                        e[j] = pg_wave_sample_array(iter + 1, n - i - 1, 1,
                                    (vec4){ p_[j] });
                    }
                    float e_ = e[0];
                    switch(iter->op) {
                        case PG_WAVE_EXPAND_ADD: case PG_WAVE_EXPAND_AVG:
                            for(j = 1; j < d_; ++j) e_ += e[j];
                            break;
                        case PG_WAVE_EXPAND_SUB:
                            for(j = 1; j < d_; ++j) e_ -= e[j];
                            break;
                        case PG_WAVE_EXPAND_MUL:
                            for(j = 1; j < d_; ++j) e_ *= e[j];
                            break;
                        case PG_WAVE_EXPAND_DIV:
                            for(j = 1; j < d_; ++j) e_ /= e[j];
                            break;
                    }
                    if(iter->op == PG_WAVE_EXPAND_AVG) e_ /= d_;
                    s += e_;
                    break;
                } }
                i = n;
                break;
            } }
        }
    }
    return s;
}

float pg_wave_sample(struct pg_wave* wave, int d, vec4 p)
{
    switch(wave->type) {
    case PG_WAVE_MODIFIER: return 0;
    case PG_WAVE_CONSTANT: return wave->constant;
    case PG_WAVE_ARRAY:
        return pg_wave_sample_array(wave->arr, wave->len, d, p);
    case PG_WAVE_FUNCTION: {
        vec4 p_;
        switch(d) {
        case 0:
            return wave->add * wave->scale;
        case 1: 
            p_[0] = (p[0] + wave->phase[0]) * wave->frequency[0];
            return (wave->func1(p_[0]) * wave->scale) + wave->add;
        case 2:
            p_[0] = (p[0] + wave->phase[0]) * wave->frequency[0];
            p_[1] = (p[1] + wave->phase[1]) * wave->frequency[1];
            return (wave->func2(p_[0], p_[1]) * wave->scale) + wave->add;
        case 3:
            p_[0] = (p[0] + wave->phase[0]) * wave->frequency[0];
            p_[1] = (p[1] + wave->phase[1]) * wave->frequency[1];
            p_[2] = (p[2] + wave->phase[2]) * wave->frequency[2];
            return (wave->func3(p_[0], p_[1], p_[2]) * wave->scale) + wave->add;
        case 4:
            p_[0] = (p[0] + wave->phase[0]) * wave->frequency[0];
            p_[1] = (p[1] + wave->phase[1]) * wave->frequency[1];
            p_[2] = (p[2] + wave->phase[2]) * wave->frequency[2];
            p_[3] = (p[3] + wave->phase[3]) * wave->frequency[3];
            return (wave->func4(p_[0], p_[1], p_[2], p_[3]) * wave->scale) + wave->add;
        }
    } }
    return 0;
}

/*  Function definitions for the built-in waves */
float sin1(float x)
{
    return sin(x * M_PI * 2);
}

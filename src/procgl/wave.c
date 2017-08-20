#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "ext/linmath.h"
#include "wave.h"

static inline void p_transform(vec4 out, vec4 in, vec4 phase, vec4 freq)
{
    vec4_mul(out, in, freq);
    vec4_add(out, out, phase);
}

/*  TODO: Make this non-recursive   */
static float pg_wave_sample_array(struct pg_wave* wave, int* end_idx, int n,
                                  unsigned d, float* p)
{
    vec4 p_ = {};
    switch(d) {
        case 0: return 0;
        case 1: p_[0] = p[0]; break;
        case 2: vec2_dup(p_, p); break;
        case 3: vec3_dup(p_, p); break;
        case 4: vec4_dup(p_, p); break;
    }
    float scale = 1, add = 0;
    float s = 0;
    int i;
    int sub_len = 1;
    struct pg_wave* iter = wave;
    for(i = 0; i < n; i += sub_len, iter = wave + i) {
        vec4 iter_p = {};
        if(iter->type == PG_WAVE_POP) {
            sub_len = 2;
            break;
        } else sub_len = 1;
        switch(iter->type) {
        default: break;
        case PG_WAVE_CONSTANT: {
            s += iter->constant * iter->scale + iter->add;
            break;
        } case PG_WAVE_PUSH: {
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            s += pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, d, iter_p)
                * iter->scale + iter->add;
            break;
        } case PG_WAVE_SWIZZ: {
            iter_p[0] = p_[iter->swizzle_x];
            iter_p[1] = p_[iter->swizzle_y];
            iter_p[2] = p_[iter->swizzle_z];
            iter_p[3] = p_[iter->swizzle_w];
            p_transform(p_, iter_p, iter->phase, iter->frequency);
            break;
        } case PG_WAVE_FM: {
            p_transform(iter_p, p_, iter->phase, (vec4){1,1,1,1});
            vec4 fm = {};
            int j;
            for(j = 0; j < 4; ++j) {
                fm[j] = pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, 4, iter_p);
            }
            vec4_mul(p_, iter_p, fm);
            vec4_mul(p_, p_, iter->frequency);
            break;
        } case PG_WAVE_PM: {
            p_transform(iter_p, p_, (vec4){}, iter->frequency);
            vec4 pm = {};
            int j;
            for(j = 0; j < 4; ++j) {
                pm[j] = pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, 4, iter_p);
            }
            vec4_add(p_, iter_p, pm);
            vec4_add(p_, p_, iter->phase);
            break;
        } case PG_WAVE_AM: {
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            float am = pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, 4, iter_p);
            scale *= am;
            break;
        } case PG_WAVE_ARRAY: {
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            s += pg_wave_sample_array(iter->arr, NULL, iter->len, d, iter_p) * iter->scale + iter->add;
            break;
        } case PG_WAVE_FUNCTION: {
            float s_ = 0;
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            int d_ = d;
            while(!(iter->dimension_mask & (1 << (d_ - 1)))) --d_;
            switch(d_) {
            case 1: s_ = iter->func1(iter_p[0]); break;
            case 2: s_ = iter->func2(iter_p[0], iter_p[1]); break;
            case 3: s_ = iter->func3(iter_p[0], iter_p[1], iter_p[2]); break;
            case 4: s_ = iter->func4(iter_p[0], iter_p[1], iter_p[2], iter_p[3]); break;
            default: break;
            }
            s += s_ * iter->scale + iter->add;
            break;
        } case PG_WAVE_MIX_FUNC: {
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            float s_ = pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, d, iter_p);
            s = iter->mix(s, s_, iter->mix_k) * iter->scale + iter->add;
            break;
        } case PG_WAVE_MODIFIER_OCTAVES: {
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            float s_ = 0;
            int j;
            float f = 1, v = 1;
            for(j = 0; j < iter->octaves; ++j) {
                s_ += v * pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, d,
                    (vec4){ iter_p[0] * f , iter_p[1] * f, iter_p[2] * f, iter_p[3] * f });
                f *= iter->ratio;
                v *= iter->decay;
            }
            s += s_ * iter->scale + iter->add;
            break;
        } case PG_WAVE_MODIFIER_SEAMLESS_1D: {
            int seamless_d = 2;
            vec4_set(iter_p, cos(p_[0] * 2 * M_PI), sin(p_[0] * 2 * M_PI),
                     0, 0);
            p_transform(iter_p, iter_p, iter->phase, iter->frequency);
            s += pg_wave_sample_array(iter + 1, &sub_len, n - i - 1,
                                      seamless_d, iter_p) * iter->scale + iter->add;
            break;
        } case PG_WAVE_MODIFIER_SEAMLESS_2D: {
            if(d < 2) return 0;
            vec4_set(iter_p, cos((p_[0] + 1) * M_PI), sin((p_[0] + 1)* M_PI),
                             cos((p_[1] + 1) * M_PI), sin((p_[1] + 1) * M_PI));
            p_transform(iter_p, iter_p, iter->phase, iter->frequency);
            vec4_scale(iter_p, iter_p, 1 / (M_PI * 2));
            s += pg_wave_sample_array(iter + 1, &sub_len, n - i - 1,
                                      4, iter_p) * iter->scale + iter->add;
            break;
        } case PG_WAVE_MODIFIER_EXPAND: {
            if(d == 1) continue;
            p_transform(iter_p, p_, iter->phase, iter->frequency);
            switch(iter->mode) {
            case PG_WAVE_EXPAND_BEFORE: {
                int j;
                float e = p_[0];
                switch(iter->op) {
                    case PG_WAVE_EXPAND_ADD: case PG_WAVE_EXPAND_AVG:
                        for(j = 1; j < d; ++j) e += p_[j];
                        break;
                    case PG_WAVE_EXPAND_SUB:
                        for(j = 1; j < d; ++j) e -= p_[j];
                        break;
                    case PG_WAVE_EXPAND_MUL:
                        for(j = 1; j < d; ++j) e *= p_[j];
                        break;
                    case PG_WAVE_EXPAND_DIV:
                        for(j = 1; j < d; ++j) e /= p_[j];
                        break;
                }
                if(iter->op == PG_WAVE_EXPAND_AVG) e /= d;
                s += pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, 1,
                        (vec4){ e }) * iter->scale + iter->add;
                break;
            } case PG_WAVE_EXPAND_AFTER: {
                float e[d];
                int j;
                for(j = 0; j < d; ++j) {
                    e[j] = pg_wave_sample_array(iter + 1, &sub_len, n - i - 1, 1,
                                (vec4){ p_[j] });
                }
                float e_ = e[0];
                switch(iter->op) {
                    case PG_WAVE_EXPAND_ADD: case PG_WAVE_EXPAND_AVG:
                        for(j = 1; j < d; ++j) e_ += e[j];
                        break;
                    case PG_WAVE_EXPAND_SUB:
                        for(j = 1; j < d; ++j) e_ -= e[j];
                        break;
                    case PG_WAVE_EXPAND_MUL:
                        for(j = 1; j < d; ++j) e_ *= e[j];
                        break;
                    case PG_WAVE_EXPAND_DIV:
                        for(j = 1; j < d; ++j) e_ /= e[j];
                        break;
                }
                if(iter->op == PG_WAVE_EXPAND_AVG) e_ /= d;
                s += e_ * iter->scale + iter->add;
                break;
            } }
            break;
        } }
    }
    if(end_idx) *end_idx = i + sub_len;
    return s * scale + add;
}

float pg_wave_sample(struct pg_wave* wave, int d, vec4 p)
{
    int a = 1;
    return pg_wave_sample_array(wave, &a, 1, d, p);
}

/*  Built-in mixing functions   */
float pg_wave_mix_min(float a, float b, float k)
{
    return MIN(a, b);
}
float pg_wave_mix_min_abs(float a, float b, float k)
{
    return MIN(fabsf(a), fabsf(b)) * SGN(a);
}
float pg_wave_mix_smin(float a, float b, float k)
{
    return smin(a, b, k);
}
float pg_wave_mix_smin_abs(float a, float b, float k)
{
    return smin(fabsf(a), fabsf(b), k) * SGN(a);
}
float pg_wave_mix_max(float a, float b, float k)
{
    return MAX(a, b);
}
float pg_wave_mix_max_abs(float a, float b, float k)
{
    return MAX(fabsf(a), fabsf(b)) * SGN(a);
}
float pg_wave_mix_lerp(float a, float b, float k)
{
    return lerp(a, b, k);
}

/*  Function definitions for the built-in waves */
float pg_wave_rand1(float x)
{
    return (float)((double)rand() / (double)RAND_MAX);
}

float pg_wave_sin1(float x)
{
    return sin(x * M_PI * 2);
}
float pg_wave_tri1(float x)
{
    return fabsf(fmodf(x, 4.0f) - 2) - 1;
}
float pg_wave_square1(float x)
{
    return (fmodf(x, 2.0f) < 1.0) * 2 - 1;
}
float pg_wave_saw1(float x)
{
    return 1.0f - floor(x);
}

float pg_wave_dist1(float x)
{
    return fabsf(x);
}
float pg_wave_dist2(float x, float y)
{
    return sqrtf(x*x + y*y);
}
float pg_wave_dist3(float x, float y, float z)
{
    return sqrtf(x*x + y*y + z*z);
}
float pg_wave_dist4(float x, float y, float z, float w)
{
    return sqrtf(x*x + y*y + z*z + w*w);
}

float pg_wave_max1(float x)
{
    return fabs(x);
}
float pg_wave_max2(float x, float y)
{
    return MAX(fabs(x), fabs(y));
}
float pg_wave_max3(float x, float y, float z)
{
    return MAX(MAX(fabs(x), fabs(y)), fabs(z));
}
float pg_wave_max4(float x, float y, float z, float w)
{
    return MAX(MAX(MAX(fabs(x), fabs(y)), fabs(z)), fabs(w));
}

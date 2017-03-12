#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "arr.h"
#include "wave.h"
#include "audio.h"
#include "ext/wavfile.h"

struct pg_audio_chunk_ref {
    struct pg_audio_chunk* chunk;
    float volume;
    int progress;
};

static SDL_AudioSpec pg_audio_spec;
static int pg_have_audio;
static ARR_T(struct pg_audio_chunk_ref) pg_audio_play_queue;

/*  The callback for SDL; buffers from the play queue, deletes sounds that
    are finished playing    */
static void pg_buffer_audio(void* udata, Uint8* stream, int len)
{
    memset(stream, pg_audio_spec.silence, len);
    int s_len = len / sizeof(float);
    float* s_stream = stream;
    struct pg_audio_chunk_ref* ref;
    int i;
    ARR_FOREACH_PTR_REV(pg_audio_play_queue, ref, i) {
        int ref_i = 0;
        int chunk_stop = ref->chunk->len - ref->progress < s_len ?
            ref->chunk->len : ref->progress + s_len;
        while(ref_i + ref->progress <= chunk_stop) {
            s_stream[ref_i] +=
                ref->chunk->samples[ref_i + ref->progress] * ref->volume;
            ++ref_i;
        }
        if(chunk_stop >= ref->chunk->len) {
            ARR_SWAPSPLICE(pg_audio_play_queue, i, 1);
        } else {
            ref->progress += ref_i;
        }
    }
}

int pg_init_audio(void)
{
    SDL_memset(&pg_audio_spec, 0, sizeof(pg_audio_spec));
    pg_audio_spec.freq = 48000;
    pg_audio_spec.format = AUDIO_F32;
    pg_audio_spec.channels = 1;
    pg_audio_spec.samples = 1024;
    pg_audio_spec.callback = pg_buffer_audio;
    SDL_ClearError();
    int audio_success = SDL_OpenAudio(&pg_audio_spec, NULL);
    if(audio_success != 0) {
        printf("Failed to init SDL audio: %s\n", SDL_GetError());
        pg_have_audio = 0;
        return 0;
    } else pg_have_audio = 1;
    SDL_PauseAudio(0);
    ARR_INIT(pg_audio_play_queue);
    return 1;
}

void pg_audio_alloc(struct pg_audio_chunk* chunk, float len)
{
    chunk->samples = malloc(sizeof(float) * (len * PG_AUDIO_SAMPLE_RATE));
    chunk->len = len * PG_AUDIO_SAMPLE_RATE;
}

void pg_audio_free(struct pg_audio_chunk* chunk)
{
    free(chunk->samples);
    chunk->samples = NULL;
    chunk->len = 0;
}

void pg_audio_generate(struct pg_audio_chunk* chunk, float len,
                       struct pg_wave* w, struct pg_audio_envelope* env)
{
    int s_len = len * PG_AUDIO_SAMPLE_RATE;
    float attack_time = floor(env->attack_time * PG_AUDIO_SAMPLE_RATE);
    float decay_time = floor(env->decay_time * PG_AUDIO_SAMPLE_RATE);
    float release_time = floor(env->release_time * PG_AUDIO_SAMPLE_RATE);
    float decay_end = attack_time + decay_time;
    float sustain_end = s_len - release_time;
    float decay = env->max - env->sustain;
    int i;
    for(i = 0; i < s_len; ++i) {
        float s = pg_wave_sample(w, 1, (float)i / PG_AUDIO_SAMPLE_RATE);
        if(i < attack_time) {
            chunk->samples[i] = s * (i / attack_time) * env->max;
        } else if(i < decay_end) {
            chunk->samples[i] =
                s * (env->max - (i - attack_time) / decay_time * decay);
        } else if(i < sustain_end) {
            chunk->samples[i] = s * env->sustain;
        } else {
            chunk->samples[i] = s *
                (env->sustain - (i - sustain_end) / release_time * env->sustain);
        }
    }
}

void pg_audio_play(struct pg_audio_chunk* chunk, float volume)
{
    struct pg_audio_chunk_ref ref = {
        .chunk = chunk,
        .volume = volume,
        .progress = 0 };
    ARR_PUSH(pg_audio_play_queue, ref);
}

void pg_audio_save(struct pg_audio_chunk* chunk, const char* filename)
{
    int16_t buf[chunk->len];
    int i;
    for(i = 0; i < chunk->len; ++i) {
        buf[i] = chunk->samples[i] * INT16_MAX;
    }
    FILE* f = wavfile_open(filename);
    wavfile_write(f, buf, chunk->len);
    wavfile_close(f);
}

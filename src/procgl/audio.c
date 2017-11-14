#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "ext/linmath.h"
#include "arr.h"
#include "wave.h"
#include "audio.h"
#include "ext/wavfile.h"
#define DR_WAV_IMPLEMENTATION
#include "ext/dr_wav.h"

struct pg_audio_chunk_ref {
    struct pg_audio_chunk* chunk;
    float volume;
    int start, len;
    int progress;
};


static SDL_AudioSpec pg_audio_spec;
static SDL_AudioDeviceID pg_audio_dev;
static int pg_have_audio;
static ARR_T(struct pg_audio_chunk_ref) pg_audio_play_queue;

/*  The callback for SDL; buffers from the play queue, deletes sounds that
    are finished playing    */
static void pg_buffer_audio(void* udata, Uint8* stream, int len)
{
    memset(stream, pg_audio_spec.silence, len);
    int s_len = len / sizeof(int16_t);
    int16_t* s_stream = (int16_t*)stream;
    struct pg_audio_chunk_ref* ref;
    int i;
    ARR_FOREACH_PTR_REV(pg_audio_play_queue, ref, i) {
        int segment = (ref->start + ref->progress) % ref->chunk->len;
        int segment_len = MIN(ref->len - ref->progress, s_len);
        int pos_to_end = MIN(ref->chunk->len - segment, segment_len);
        int pos_from_beginning = MAX(0, segment_len - pos_to_end);
        int first_end = segment + pos_to_end;
        int ref_i = segment;
        int stream_i = 0;
        ref->progress += segment_len;
        for(ref_i = segment; ref_i < first_end; ++ref_i, ++stream_i) {
            s_stream[stream_i] += ref->chunk->samples[ref_i] * ref->volume;
        }
        for(ref_i = 0; ref_i < pos_from_beginning; ++ref_i, ++stream_i) {
            s_stream[stream_i] += ref->chunk->samples[ref_i] * ref->volume;
        }
        if(ref->progress >= ref->len) ARR_SWAPSPLICE(pg_audio_play_queue, i, 1);
    }
}

int pg_init_audio(void)
{
    SDL_AudioSpec spec;
    memset(&spec, 0, sizeof(spec));
    memset(&pg_audio_spec, 0, sizeof(pg_audio_spec));
    spec.freq = PG_AUDIO_SAMPLE_RATE;
    spec.format = AUDIO_S16;
    spec.channels = 1;
    spec.samples = 1024;
    spec.callback = pg_buffer_audio;
    SDL_ClearError();
    ARR_INIT(pg_audio_play_queue);
    pg_audio_dev = SDL_OpenAudioDevice(NULL, 0, &spec, &pg_audio_spec, 0);
    if(!pg_audio_dev) {
        printf("Failed to init SDL audio: %s\n", SDL_GetError());
        pg_have_audio = 0;
        return 0;
    } else pg_have_audio = 1;
    SDL_PauseAudioDevice(pg_audio_dev, 0);
    return 1;
}

void pg_audio_alloc(struct pg_audio_chunk* chunk, float len)
{
    chunk->samples = malloc(sizeof(int16_t) * (len * PG_AUDIO_SAMPLE_RATE));
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
    int s_len = len * (float)PG_AUDIO_SAMPLE_RATE;
    float attack_time = floor(env->attack_time * PG_AUDIO_SAMPLE_RATE);
    float decay_time = floor(env->decay_time * PG_AUDIO_SAMPLE_RATE);
    float release_time = floor(env->release_time * PG_AUDIO_SAMPLE_RATE);
    float decay_end = attack_time + decay_time;
    float sustain_end = s_len - release_time;
    float decay = env->max - env->sustain;
    int i;
    for(i = 0; i < s_len; ++i) {
        float s = pg_wave_sample(w, 1, (vec4){ (float)i / PG_AUDIO_SAMPLE_RATE });
        if(i < attack_time) {
            s = s * (i / attack_time) * env->max;
        } else if(i < decay_end) {
            s = s * (env->max - (i - attack_time) / decay_time * decay);
        } else if(i < sustain_end) {
            s = s * env->sustain;
        } else {
            s = s * (env->sustain - (i - sustain_end) / release_time * env->sustain);
        }
        s = clamp(s, -1.0f, 1.0f);
        chunk->samples[i] = (int16_t)(s * (INT16_MAX - 1));
    }
}

void pg_audio_play(struct pg_audio_chunk* chunk, float volume)
{
    struct pg_audio_chunk_ref ref = {
        .chunk = chunk,
        .volume = volume,
        .start = 0, .len = chunk->len,
        .progress = 0 };
    ARR_PUSH(pg_audio_play_queue, ref);
}

void pg_audio_loop(struct pg_audio_chunk* chunk, float volume,
                   float start, float len)
{
    struct pg_audio_chunk_ref ref = {
        .chunk = chunk,
        .volume = volume,
        .start = (int)(start * PG_AUDIO_SAMPLE_RATE) % chunk->len,
        .len = (int)(len * PG_AUDIO_SAMPLE_RATE),
        .progress = 0 };
    ARR_PUSH(pg_audio_play_queue, ref);
}

void pg_audio_save(struct pg_audio_chunk* chunk, const char* filename)
{
    FILE* f = wavfile_open(filename);
    wavfile_write(f, chunk->samples, chunk->len);
    wavfile_close(f);
}

void pg_audio_load_wav(struct pg_audio_chunk* chunk, const char* filename)
{
    drwav wav;
    if(!drwav_init_file(&wav, filename)) {
        printf("Failed to load file: %s\n", filename);
        *chunk = (struct pg_audio_chunk){};
        return;
    }
    if(wav.sampleRate != PG_AUDIO_SAMPLE_RATE) {
        printf("Warning! Unexpected sample rate in %s\n", filename);
    }
    chunk->len = wav.totalSampleCount;
    chunk->samples = malloc(wav.totalSampleCount * sizeof(int16_t));
    drwav_read_s16(&wav, wav.totalSampleCount, chunk->samples);
}

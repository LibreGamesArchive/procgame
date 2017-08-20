#define PG_AUDIO_SAMPLE_RATE 44100

struct pg_audio_chunk {
    int16_t* samples;
    int len;
};

struct pg_audio_envelope {
    float attack_time;
    float max;
    float decay_time;
    float sustain;
    float release_time;
};

int pg_init_audio(void);

void pg_audio_alloc(struct pg_audio_chunk* chunk, float len);
void pg_audio_free(struct pg_audio_chunk* chunk);
void pg_audio_generate(struct pg_audio_chunk* chunk, float len,
                       struct pg_wave* w, struct pg_audio_envelope* env);
void pg_audio_play(struct pg_audio_chunk* chunk, float volume);
void pg_audio_save(struct pg_audio_chunk* chunk, const char* filename);

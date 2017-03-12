#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"

struct example_game_renderer {
    struct pg_shader shader_text;
};

struct example_game_assets {
    struct pg_texture font;
    struct pg_audio_chunk sound;
};

struct example_game_data {
    struct example_game_renderer rend;
    struct example_game_assets assets;
    const uint8_t* kb_state;
    vec2 player_pos, player_vel;
};

void example_game_tick(struct pg_game_state*);
void example_game_draw(struct pg_game_state*);
void example_game_deinit(void*);

static void example_game_gen_audio(struct pg_audio_chunk* snd)
{
    struct pg_wave wave;
    pg_wave_init_sine(&wave);
    wave.frequency[0] = 440;
    wave.scale = 0.25;
    struct pg_audio_envelope env = {
        .attack_time = 0.01,
        .max = 2,
        .decay_time = 0.01,
        .sustain = 1,
        .release_time = 0.2 };
    pg_audio_alloc(snd, 0.25);
    pg_audio_generate(snd, 0.25, &wave, &env);
    pg_audio_save(snd, "test.wav");
}

void example_game_start(struct pg_game_state* state)
{
    pg_game_state_init(state, pg_time(), 30, 3);
    struct example_game_data* d = malloc(sizeof(*d));
    d->kb_state = SDL_GetKeyboardState(NULL);
    vec2_set(d->player_pos, 100, 100);
    vec2_set(d->player_vel, 0, 0);
    example_game_gen_audio(&d->assets.sound);
    pg_texture_init_from_file(&d->assets.font, "font_8x8.png", NULL, 0, -1);
    pg_texture_set_atlas(&d->assets.font, 8, 8);
    pg_shader_text(&d->rend.shader_text);
    pg_shader_text_set_font(&d->rend.shader_text, &d->assets.font);
    state->data = d;
    state->tick = example_game_tick;
    state->draw = example_game_draw;
    state->deinit = example_game_deinit;
}

void example_game_tick(struct pg_game_state* state)
{
    struct example_game_data* d = state->data;
    if(d->kb_state[SDL_SCANCODE_LEFT]) {
        d->player_vel[0] -= 10;
    }
    if(d->kb_state[SDL_SCANCODE_RIGHT]) {
        d->player_vel[0] += 10;
    }
    if(d->kb_state[SDL_SCANCODE_UP]) {
        d->player_vel[1] -= 10;
    }
    if(d->kb_state[SDL_SCANCODE_DOWN]) {
        d->player_vel[1] += 10;
    }
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->tick = NULL;
        else if(e.type == SDL_KEYDOWN) {
            pg_audio_play(&d->assets.sound, 1);
        }
    }
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
}

void example_game_draw(struct pg_game_state* state)
{
    struct example_game_data* d = state->data;
    float t = state->tick_over;
    pg_screen_dst();
    vec2 vel_lerp = { d->player_vel[0] * t, d->player_vel[1] * t };
    vec2 pos = { d->player_pos[0] + vel_lerp[0] - 16,
                 d->player_pos[1] + vel_lerp[1] - 16 };
    pg_shader_begin(&d->rend.shader_text, NULL);
    pg_shader_text_write(&d->rend.shader_text, "@", pos, (vec2){ 32, 32 }, 0);
}

void example_game_deinit(void* data)
{
    struct example_game_data* d = data;
    pg_shader_deinit(&d->rend.shader_text);
    pg_texture_deinit(&d->assets.font);
    free(d);
}

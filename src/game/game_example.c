#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"

struct example_game_renderer {
    struct pg_shader shader_2d;
};

struct example_game_assets {
    struct pg_texture font;
    struct pg_model quad;
};

struct example_game_data {
    struct example_game_renderer rend;
    struct example_game_assets assets;
    vec2 player_pos, player_vel;
};

void example_game_update(struct pg_game_state*, float);
void example_game_tick(struct pg_game_state*);
void example_game_draw(struct pg_game_state*);
void example_game_deinit(void*);

void example_game_start(struct pg_game_state* state)
{
    pg_game_state_init(state, pg_time(), 30, 3);
    struct example_game_data* d = malloc(sizeof(*d));
    vec2_set(d->player_pos, 0, 0);
    vec2_set(d->player_vel, 0, 0);
    pg_texture_init_from_file(&d->assets.font, "res/font_8x8.png", NULL);
    pg_texture_bind(&d->assets.font, 0, -1);
    pg_texture_set_atlas(&d->assets.font, 8, 8);
    pg_shader_2d(&d->rend.shader_2d);
    pg_shader_2d_texture(&d->rend.shader_2d, &d->assets.font);
    /*  Basic centered quad */
    pg_model_init(&d->assets.quad);
    pg_model_quad(&d->assets.quad, (vec2){ 1, 1 });
    mat4 transform;
    mat4_identity(transform);
    mat4_scale_aniso(transform, transform, 1, -1, 0);
    pg_model_transform(&d->assets.quad, transform);
    pg_model_reserve_component(&d->assets.quad,
        PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    vec4_t* color;
    float* f;
    int i;
    ARR_FOREACH_PTR(d->assets.quad.height, f, i) {
        *f = 1.0f;
    }
    ARR_FOREACH_PTR(d->assets.quad.color, color, i) {
        vec4_set(color->v, 1, 1, 1, 1);
    }
    pg_shader_buffer_model(&d->rend.shader_2d, &d->assets.quad);
    /*  Fill in state structure */
    state->data = d;
    state->tick = example_game_tick;
    state->draw = example_game_draw;
    state->deinit = example_game_deinit;
}

void example_game_tick(struct pg_game_state* state)
{
    struct example_game_data* d = state->data;
    pg_poll_input();
    if(pg_user_exit()) state->running = 0;
    if(pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HELD)) {
        d->player_vel[0] -= 0.01;
    }
    if(pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HELD)) {
        d->player_vel[0] += 0.01;
    }
    if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HELD)) {
        d->player_vel[1] -= 0.01;
    }
    if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HELD)) {
        d->player_vel[1] += 0.01;
    }
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
    pg_flush_input();
}

void example_game_draw(struct pg_game_state* state)
{
    struct example_game_data* d = state->data;
    float t = state->tick_over;
    pg_screen_dst();
    vec2 vel_lerp = { d->player_vel[0] * t, d->player_vel[1] * t };
    vec2 pos = { d->player_pos[0] + vel_lerp[0],
                 d->player_pos[1] + vel_lerp[1] };
    pg_shader_begin(&d->rend.shader_2d, NULL);
    pg_shader_2d_resolution(&d->rend.shader_2d, (vec2){ 1, 1 });
    pg_shader_2d_transform(&d->rend.shader_2d, pos, (vec2){ 0.1, 0.1 }, 0);
    pg_shader_2d_set_light(&d->rend.shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_tex_frame(&d->rend.shader_2d, '@' - 32);
    pg_model_begin(&d->assets.quad, &d->rend.shader_2d);
    pg_model_draw(&d->assets.quad, NULL);
}

void example_game_deinit(void* data)
{
    struct example_game_data* d = data;
    pg_shader_deinit(&d->rend.shader_2d);
    pg_texture_deinit(&d->assets.font);
    pg_model_deinit(&d->assets.quad);
    free(d);
}

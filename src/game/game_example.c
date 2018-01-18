#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"

struct example_game_assets {
    struct pg_texture font;
    struct pg_model quad;
};

struct example_game_data {
    struct pg_renderer rend;
    struct pg_renderpass test_pass;
    struct pg_rendergroup test_group;
    struct example_game_assets assets;
    vec2 player_pos, player_vel;
};

void example_game_update(struct pg_game_state*, float);
void example_game_tick(struct pg_game_state*);
void example_game_draw(struct pg_game_state*);
void example_game_deinit(void*);

static int drawformat_2d(const struct pg_uniform* unis, const struct pg_shader* shader,
                         const mat4* mats, const GLint* idx)
{
    glUniformMatrix4fv(idx[0], 1, GL_FALSE, *unis[0].m);
    glUniform4f(idx[1], unis[1].f[0], unis[1].f[1], unis[1].f[2], unis[1].f[3]);
    return 2;
}

void example_game_start(struct pg_game_state* state)
{
    pg_game_state_init(state, pg_time(), 30, 3);
    struct example_game_data* d = malloc(sizeof(*d));
    pg_renderer_init(&d->rend);
    pg_renderpass_init(&d->rend, &d->test_pass, "2d", NULL, GL_COLOR_BUFFER_BIT);
    vec2_set(d->player_pos, 0, 0);
    vec2_set(d->player_vel, 0, 0);
    pg_texture_init_from_file(&d->assets.font, "res/font_8x8.png", NULL);
    pg_texture_bind(&d->assets.font, 0, -1);
    pg_texture_set_atlas(&d->assets.font, 8, 8);
    /*  Basic centered quad */
    pg_model_init(&d->assets.quad);
    pg_model_quad_2d(&d->assets.quad, (vec2){ 1, 1 });
    mat4 transform;
    mat4_identity(transform);
    mat4_scale_aniso(transform, transform, 1, 1, 0);
    pg_model_transform(&d->assets.quad, transform);
    pg_model_buffer(&d->assets.quad);
    pg_rendergroup_init(&d->test_group, &d->assets.quad);
    d->test_group.tex[0] = &d->assets.font;
    pg_rendergroup_texture(&d->test_group, 1, &d->assets.font);
    pg_rendergroup_drawformat(&d->test_group, drawformat_2d, 2,
                              "pg_matrix_model", "pg_tex_rect_0");
    struct pg_uniform test_draw[2] = {};
    mat4_identity(test_draw[0].m);
    vec4_set(test_draw[1].f, 0, 0, 1, 1);
    pg_rendergroup_add_draw(&d->test_group, 2, test_draw);
    mat4_translate(test_draw[0].m, 0.5, 0, 0);
    vec4_set(test_draw[1].f, 0, 0, 0.5, 0.5);
    pg_rendergroup_add_draw(&d->test_group, 2, test_draw);
    ARR_PUSH(d->rend.passes, &d->test_pass);
    ARR_PUSH(d->test_pass.groups, &d->test_group);
    mat4_ortho(d->test_pass.mats[PG_VIEW_MATRIX], -1, 1, 1, -1, -1, 1);
    pg_renderpass_uniform(&d->test_pass, "ambient_color", PG_VEC3,
                           &(struct pg_uniform){ .f = { 1, 1, 1 } });
    pg_renderpass_uniform(&d->test_pass, "color_mod", PG_VEC4,
                           &(struct pg_uniform){ .f = { 1, 1, 1, 1 } });
    pg_renderpass_uniform(&d->test_pass, "tex_weight", PG_FLOAT,
                           &(struct pg_uniform){ .f = { 1 } });
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
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDisable(GL_CULL_FACE);
    pg_renderer_draw_frame(&d->rend);
}

void example_game_deinit(void* data)
{
    struct example_game_data* d = data;
    pg_texture_deinit(&d->assets.font);
    pg_model_deinit(&d->assets.quad);
    free(d);
}

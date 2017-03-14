#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/ext/noise1234.h"

struct fps_game_renderer {
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    struct pg_shader shader_3d;
};

struct fps_game_assets {
    struct pg_model floor_model;
    struct pg_texture floor_tex;
};

struct fps_game_data {
    struct fps_game_renderer rend;
    struct fps_game_assets assets;
    const uint8_t* kb_state;
    vec3 player_pos, player_vel;
    vec2 player_dir;
};

static void fps_game_generate_assets(struct fps_game_data* d);

static void fps_game_tick(struct pg_game_state* state);
static void fps_game_draw(struct pg_game_state* state);
static void fps_game_deinit(void* data);

void fps_game_start(struct pg_game_state* state)
{
    pg_game_state_init(state, pg_time(), 60, 3);
    struct fps_game_data* d = malloc(sizeof(*d));
    int sw, sh;
    pg_screen_size(&sw, &sh);
    pg_gbuffer_init(&d->rend.gbuf, sw, sh);
    pg_gbuffer_bind(&d->rend.gbuf, 20, 21, 22, 23);
    pg_viewer_init(&d->rend.view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        (vec2){ sw, sh }, (vec2){ 0.1, 200});
    pg_shader_3d(&d->rend.shader_3d);
    fps_game_generate_assets(d);
    pg_shader_3d_set_texture(&d->rend.shader_3d, &d->assets.floor_tex);
    d->kb_state = SDL_GetKeyboardState(NULL);
    vec2_set(d->player_pos, 0, 0);
    vec2_set(d->player_vel, 0, 0);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    state->data = d;
    state->tick = fps_game_tick;
    state->draw = fps_game_draw;
    state->deinit = fps_game_deinit;
}

static void fps_game_tick(struct pg_game_state* state)
{
    struct fps_game_data* d = state->data;
    int mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    vec2_set(d->player_dir,
             d->player_dir[0] - (mx * 0.0005f),
             d->player_dir[1] - (my * 0.0005f));
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->tick = NULL;
    }
    if(d->kb_state[SDL_SCANCODE_A]) {
        d->player_vel[0] -= 0.1 * sin(d->player_dir[0]);
        d->player_vel[1] += 0.1 * cos(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_D]) {
        d->player_vel[0] += 0.1 * sin(d->player_dir[0]);
        d->player_vel[1] -= 0.1 * cos(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_W]) {
        d->player_vel[0] += 0.1 * cos(d->player_dir[0]);
        d->player_vel[1] += 0.1 * sin(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_S]) {
        d->player_vel[0] -= 0.1 * cos(d->player_dir[0]);
        d->player_vel[1] -= 0.1 * sin(d->player_dir[0]);
    }
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
}

static void fps_game_draw(struct pg_game_state* state)
{
    struct fps_game_data* d = state->data;
    float t = state->tick_over;
    vec2 vel_lerp = { d->player_vel[0] * t, d->player_vel[1] * t };
    vec3 pos = { d->player_pos[0] + vel_lerp[0],
                 d->player_pos[1] + vel_lerp[1], 2 };
    pg_viewer_set(&d->rend.view, pos, d->player_dir);
    pg_gbuffer_dst(&d->rend.gbuf);
    pg_shader_begin(&d->rend.shader_3d, &d->rend.view);
    pg_model_begin(&d->assets.floor_model);
    mat4 model_transform;
    mat4_identity(model_transform);
    pg_model_draw(&d->assets.floor_model, &d->rend.shader_3d, model_transform);
    pg_gbuffer_begin_light(&d->rend.gbuf, &d->rend.view);
    pg_gbuffer_draw_light(&d->rend.gbuf,
        (vec4){ 0, 0, 1, 20 },
        (vec3){ 1, 0.25, 0.25 });
    pg_screen_dst();
    pg_gbuffer_finish(&d->rend.gbuf, (vec3){ 0.1, 0.1, 0.1 });
}

static void fps_game_deinit(void* data)
{
    struct fps_game_data* d = data;
    pg_shader_deinit(&d->rend.shader_3d);
    pg_gbuffer_deinit(&d->rend.gbuf);
    pg_model_deinit(&d->assets.floor_model);
    pg_texture_deinit(&d->assets.floor_tex);
    free(d);
}

static void fps_game_generate_floor_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128, 1, 2);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            float _x = fabs(x - 64);
            float _y = fabs(y - 64);
            float dist = (_x > _y) ? _x : _y;
            float s = noise2(x / 8.0f, y / 8.0f) * 0.5 + 0.5;
            float c = s * 32 + 150;
            if(dist < 56) {
                tex->pixels[x + y * tex->w] =
                    (struct pg_texture_pixel){ c, c, c, 255 };
                tex->normals[x + y * tex->w].h = s * 255.0f;
            } else {
                float scale = 1 - ((dist - 56) / 8);
                c *= scale * 0.5 + 0.5;
                s *= scale;
                tex->pixels[x + y * tex->w] =
                    (struct pg_texture_pixel){ c, c, c, 255 };
                tex->normals[x + y * tex->w].h = s * 255.0f;
            }
        }
    }
    pg_texture_generate_normals(tex, 4);
    pg_texture_buffer(tex);
}

static void fps_game_generate_assets(struct fps_game_data* d)
{
    /*  Generating the floor model  */
    pg_model_quad(&d->assets.floor_model, 10, 10);
    mat4 transform;
    mat4_identity(transform);
    mat4_rotate_X(transform, transform, -M_PI / 2);
    mat4_scale_aniso(transform, transform, 10, 10, 10);
    pg_model_transform(&d->assets.floor_model, transform);
    pg_model_precalc_verts(&d->assets.floor_model);
    pg_model_buffer(&d->assets.floor_model, &d->rend.shader_3d);
    /*  Generating the floor texture    */
    fps_game_generate_floor_texture(&d->assets.floor_tex);
}


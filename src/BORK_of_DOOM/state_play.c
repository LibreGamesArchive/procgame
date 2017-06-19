#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/wave_defs.h"
#include "bork.h"
#include "map_area.h"

/*  The Big Orbital Nonhuman Zone or "BONZ" consists of some main parts:
    Microgravity Utility Transit Tunnel (MUTT)
    Command Station
    Offices
    Warehouse
    Union Hall
    Infirmary
    Recreation Center   (dog park)
    Science labs
    Cafeteria/kitchens
    Pump/Electrical/Thrust Section (PETS)

    they are arranged vertically on top of one another, with varying
    widths based on the shape of the overall station, except for the MUTT
    which runs all the way along the length of the station on the inside. */

struct bork_play_data {
    struct bork_game_core* core;
    struct bork_map map;
    const uint8_t* kb_state;
    vec2 mouse_motion;
    vec3 player_pos, player_vel;
    vec2 player_dir;
    float player_speed;
};

static void bork_play_update(struct pg_game_state* state);
static void bork_play_tick(struct pg_game_state* state);
static void bork_play_draw(struct pg_game_state* state);
static void bork_play_deinit(void* data);

static void bork_play_generate_map(struct bork_play_data* d, int w, int h, int l);

void bork_play_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 3);
    struct bork_play_data* d = malloc(sizeof(*d));
    d->core = core;
    d->kb_state = SDL_GetKeyboardState(NULL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    vec2_set(d->mouse_motion, 0, 0);
    /*  Generate the BONZ station   */
    bork_play_generate_map(d, 32, 32, 128);
    /*  Initialize the player data  */
    vec2_set(d->player_dir, 0, 0);
    vec3_set(d->player_pos, 0, 0, 1.5);
    vec3_set(d->player_vel, 0, 0, 0);
    d->player_speed = 0.02;
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = bork_play_update;
    state->tick = bork_play_tick;
    state->draw = bork_play_draw;
    state->deinit = bork_play_deinit;
}

static void bork_play_update(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    int mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    d->mouse_motion[0] -= mx * 0.0005f;
    d->mouse_motion[1] -= my * 0.0005f;
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->tick = NULL;
    }
    if(d->kb_state[SDL_SCANCODE_A]) {
        d->player_vel[0] -= d->player_speed * sin(d->player_dir[0]);
        d->player_vel[1] += d->player_speed * cos(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_D]) {
        d->player_vel[0] += d->player_speed * sin(d->player_dir[0]);
        d->player_vel[1] -= d->player_speed * cos(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_W]) {
        d->player_vel[0] += d->player_speed * cos(d->player_dir[0]);
        d->player_vel[1] += d->player_speed * sin(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_S]) {
        d->player_vel[0] -= d->player_speed * cos(d->player_dir[0]);
        d->player_vel[1] -= d->player_speed * sin(d->player_dir[0]);
    }
}

static void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    vec2_set(d->player_dir,
             d->player_dir[0] + d->mouse_motion[0],
             d->player_dir[1] + d->mouse_motion[1]);
    vec2_set(d->mouse_motion, 0, 0);
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
}

static void bork_play_draw(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    /*  Interpolation   */
    float t = state->tick_over;
    vec2 vel_lerp = { d->player_vel[0] * t, d->player_vel[1] * t };
    vec3 pos = { d->player_pos[0] + vel_lerp[0],
                 d->player_pos[1] + vel_lerp[1], 2.5 };
    vec2 dir = { d->player_dir[0] + d->mouse_motion[0],
                 d->player_dir[1] + d->mouse_motion[1] };
    pg_viewer_set(&d->core->view, pos, dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->core->gbuf);
    pg_shader_begin(&d->core->shader_3d, &d->core->view);
    pg_model_begin(&d->core->map_model, &d->core->shader_3d);
    mat4 model_transform;
    mat4_identity(model_transform);
    pg_model_draw(&d->core->map_model, model_transform);
    /*  Lighting    */
    pg_gbuffer_begin_light(&d->core->gbuf, &d->core->view);
    pg_gbuffer_draw_light(&d->core->gbuf,
        (vec4){ pos[0], pos[1], pos[2], 16 },
        (vec3){ 1, 1, 1 });
    pg_screen_dst();
    pg_gbuffer_finish(&d->core->gbuf, (vec3){ 0.1, 0.1, 0.1 });
    /*  Overlay */
    pg_shader_begin(&d->core->shader_text, NULL);
    char bork_str[10];
    snprintf(bork_str, 10, "FPS: %d", (int)pg_framerate());
    pg_shader_text_write(&d->core->shader_text, bork_str,
        (vec2){ 0, 0 }, (vec2){ 8, 8 }, 0.25);
}

static void bork_play_deinit(void* data)
{
    struct bork_play_data* d = data;
    bork_map_deinit(&d->map);
    free(d);
}

/*  The Big Magic Bork Station Generation Function  */
static void bork_play_generate_map(struct bork_play_data* d, int w, int l, int h)
{
    bork_map_init(&d->map, w, l, h);
    int x, y, z;
    for(x = 0; x < w; ++x) {
        for(y = 0; y < l; ++y) {
            for(z = 0; z < h; ++z) {
                if((rand() % 20 == 0) || (z == 12) || (x < 8 && (z == 0)) || (x >= 8 && z <= x - 8)) {
                    bork_map_set_tile(&d->map, x, y, z, (struct bork_tile) {
                        BORK_TILE_HULL });
                } else {
                    bork_map_set_tile(&d->map, x, y, z, (struct bork_tile) {
                        BORK_TILE_VAC });
                }
            }
        }
    }
    bork_map_generate_model(&d->map, &d->core->map_model, &d->core->env_atlas);
    pg_shader_buffer_model(&d->core->shader_3d, &d->core->map_model);
}

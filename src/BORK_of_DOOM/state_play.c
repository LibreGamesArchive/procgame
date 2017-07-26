#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/wave_defs.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "bullet.h"
#include "physics.h"

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
    vec3 quad_pos;
    struct bork_entity plr;
    float player_speed;
    ARR_T(struct bork_bullet) bullets;
};

static void bork_play_update(struct pg_game_state* state);
static void bork_play_tick(struct pg_game_state* state);
static void bork_play_draw(struct pg_game_state* state);
static void bork_play_deinit(void* data);

void bork_play_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 2);
    struct bork_play_data* d = malloc(sizeof(*d));
    d->core = core;
    d->kb_state = SDL_GetKeyboardState(NULL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    vec2_set(d->mouse_motion, 0, 0);
    /*  Generate the BONZ station   */
    bork_map_init(&d->map, core);
    bork_map_load_from_file(&d->map, "test.bork_map");
    bork_map_init_model(&d->map);
    pg_shader_buffer_model(&d->core->shader_sprite, &d->map.door_model);
    /*  Initialize the player data  */
    bork_entity_init(&d->plr, (vec3){ 0.5, 0.5, 0.9 });
    vec3_set(d->plr.pos, 32, 32, 40);
    core->plr = &d->plr;
    d->player_speed = 0.02;
    d->map.plr = &d->plr;
    ARR_INIT(d->bullets);
    vec3_set(d->quad_pos, 0, 0, 0);
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
        if(e.type == SDL_QUIT) state->running = 0;
        else if(e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            vec3 bullet_dir;
            spherical_to_cartesian(bullet_dir, (vec2){ d->plr.dir[0] - M_PI,
                                                       d->plr.dir[1] - (M_PI * 0.5) });
            vec3_scale(bullet_dir, bullet_dir, 1);
            struct bork_bullet new_bullet = {};
            vec3_dup(new_bullet.pos, d->plr.pos);
            new_bullet.pos[0] += 0.2 * sin(d->plr.dir[0]);
            new_bullet.pos[1] -= 0.2 * cos(d->plr.dir[0]);
            new_bullet.pos[2] += 0.75;
            vec3_dup(new_bullet.dir, bullet_dir);
            ARR_PUSH(d->bullets, new_bullet);
        } else if(e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_F) {
            struct bork_entity new_ent = { .type = BORK_ENTITY_ENEMY };
            vec3_dup(new_ent.pos, d->plr.pos);
            vec3_set(new_ent.size, 1, 1, 1);
            vec2_set(new_ent.dir, d->plr.dir[0], 0);
            new_ent.dir_frames = 4;
            new_ent.front_frame = 192;
            enum bork_area area = bork_map_get_area(&d->map,
                                                    (int)new_ent.pos[0] * 0.5,
                                                    (int)new_ent.pos[1] * 0.5,
                                                    (int)new_ent.pos[2] * 0.5);
            ARR_PUSH(d->map.enemies[area], new_ent);
        } else if(e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_G) {
            struct bork_entity new_ent = { .type = BORK_ENTITY_ENEMY };
            vec3_dup(new_ent.pos, d->plr.pos);
            vec3_set(new_ent.size, 0.5, 0.5, 0.8);
            vec2_set(new_ent.dir, d->plr.dir[0], 0);
            new_ent.dir_frames = 8;
            new_ent.front_frame = 208;
            enum bork_area area = bork_map_get_area(&d->map,
                                                    (int)new_ent.pos[0] * 0.5,
                                                    (int)new_ent.pos[1] * 0.5,
                                                    (int)new_ent.pos[2] * 0.5);
            ARR_PUSH(d->map.enemies[area], new_ent);
        }
    }
}

static struct bork_entity* plr;
static int bork_entity_sort_fn(const void* av, const void* bv)
{
    const struct bork_entity* a = av;
    const struct bork_entity* b = bv;
    float a_dist, b_dist;
    vec3 ent_to_plr;
    vec3_sub(ent_to_plr, a->pos, plr->pos);
    a_dist = vec3_len(ent_to_plr);
    vec3_sub(ent_to_plr, b->pos, plr->pos);
    b_dist = vec3_len(ent_to_plr);
    if(fabs(a_dist - b_dist) < 0.001) return 0;
    else return (a_dist > b_dist);
}

static int bork_bullet_sort_fn(const void* av, const void* bv)
{
    const struct bork_bullet* a = av;
    const struct bork_bullet* b = bv;
    float a_dist, b_dist;
    vec3 ent_to_plr;
    vec3_sub(ent_to_plr, a->pos, plr->pos);
    a_dist = vec3_len(ent_to_plr);
    vec3_sub(ent_to_plr, b->pos, plr->pos);
    b_dist = vec3_len(ent_to_plr);
    if(fabs(a_dist - b_dist) < 0.001) return 0;
    else return (a_dist > b_dist);
}

static void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    if(d->kb_state[SDL_SCANCODE_SPACE] && d->plr.ground) {
        d->plr.vel[2] = 0.3;
        d->plr.ground = 0;
    }
    float move_speed = d->player_speed * (d->plr.ground ? 1 : 0.15);
    if(d->kb_state[SDL_SCANCODE_A]) {
        d->plr.vel[0] -= move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * cos(d->plr.dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_D]) {
        d->plr.vel[0] += move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * cos(d->plr.dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_W]) {
        d->plr.vel[0] += move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * sin(d->plr.dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_S]) {
        d->plr.vel[0] -= move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * sin(d->plr.dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_Q]) {
        vec3 bullet_dir;
        spherical_to_cartesian(bullet_dir, (vec2){ d->plr.dir[0] - M_PI,
                                                   d->plr.dir[1] - (M_PI * 0.5) });
        vec3_scale(bullet_dir, bullet_dir, 1);
        struct bork_bullet new_bullet = {};
        vec3_dup(new_bullet.pos, d->plr.pos);
        new_bullet.pos[0] += 0.2 * sin(d->plr.dir[0]);
        new_bullet.pos[1] -= 0.2 * cos(d->plr.dir[0]);
        new_bullet.pos[2] += 0.75;
        vec3_dup(new_bullet.dir, bullet_dir);
        ARR_PUSH(d->bullets, new_bullet);
    }
    /*  Player update   */
    vec2_set(d->plr.dir,
             d->plr.dir[0] + d->mouse_motion[0],
             d->plr.dir[1] + d->mouse_motion[1]);
    d->plr.dir[0] = fmodf(d->plr.dir[0], M_PI * 2.0f);
    vec2_set(d->mouse_motion, 0, 0);
    bork_entity_update(&d->plr, &d->map);
    /*  Map update  (doors and stuff)   */
    bork_map_update_area(&d->map, 0, &d->plr);
    plr = &d->plr;
    ARR_SORT(d->map.enemies[0], bork_entity_sort_fn);
    /*  Bullets update  */
    struct bork_bullet* blt;
    int i;
    ARR_FOREACH_PTR_REV(d->bullets, blt, i) {
        if(blt->dead_ticks == 1) {
            ARR_SWAPSPLICE(d->bullets, i, 1);
            continue;
        } else if(blt->dead_ticks) --blt->dead_ticks;
        else bork_bullet_move(blt, &d->map);
    }
    //ARR_SORT(d->bullets, bork_bullet_sort_fn);
}

static void bork_play_draw(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    /*  Interpolation   */
    float t = state->tick_over;
    vec3 vel_lerp, draw_pos;
    vec2 draw_dir;
    vec3_scale(vel_lerp, d->plr.vel, t);
    vec3_add(draw_pos, d->plr.pos, vel_lerp);
    vec3_add(draw_pos, draw_pos, (vec3){ 0, 0, 0.8 });
    vec2_add(draw_dir, d->plr.dir, d->mouse_motion);
    pg_viewer_set(&d->core->view, draw_pos, draw_dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->core->gbuf);
    bork_map_draw_area(&d->map, 0);
    pg_shader_sprite_set_texture(&d->core->shader_sprite, &d->core->bullet_tex);
    pg_shader_sprite_set_tex_frame(&d->core->shader_sprite, 0);
    pg_shader_sprite_set_mode(&d->core->shader_sprite, PG_SPRITE_SPHERICAL);
    pg_shader_begin(&d->core->shader_sprite, &d->core->view);
    pg_model_begin(&d->core->bullet_model, &d->core->shader_sprite);
    mat4 bullet_transform;
    struct bork_bullet* blt;
    int i;
    ARR_FOREACH_PTR(d->bullets, blt, i) {
        if(blt->dead_ticks) {
            struct bork_light blt_light = {
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2],
                         ((float)blt->dead_ticks / 10.0f) * 3.0f },
                .color = { 0.8f, 0.8f, 0 } };
            ARR_PUSH(d->core->lights_buf, blt_light);
            continue;
        }
        vec3 blt_lerp;
        vec3_scale(blt_lerp, blt->dir, state->tick_over);
        vec3_add(blt_lerp, blt_lerp, blt->pos);
        mat4_translate(bullet_transform, blt_lerp[0], blt_lerp[1], blt_lerp[2]);
        pg_model_draw(&d->core->bullet_model, bullet_transform);
    }
    /*  Lighting    */
    pg_gbuffer_begin_light(&d->core->gbuf, &d->core->view);
    struct bork_light* light;
    ARR_FOREACH_PTR(d->core->lights_buf, light, i) {
        pg_gbuffer_draw_light(&d->core->gbuf, light->pos, light->color);
    }
    ARR_TRUNCATE(d->core->lights_buf, 0);
    /*pg_gbuffer_draw_light(&d->core->gbuf,
        (vec4){ d->plr.pos[0], d->plr.pos[1], d->plr.pos[2], 6 },
        (vec3){ 1, 1, 1 }); */
    pg_screen_dst();
    pg_gbuffer_finish(&d->core->gbuf, (vec3){ 0.025, 0.025, 0.025 });
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

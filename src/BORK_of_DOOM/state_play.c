#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "bullet.h"
#include "physics.h"
#include "game_states.h"

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

static void bork_play_update(struct pg_game_state* state);
static void bork_play_tick(struct pg_game_state* state);
static void bork_play_draw(struct pg_game_state* state);

static void bork_play_deinit(void* data)
{
    struct bork_play_data* d = data;
    bork_map_deinit(&d->map);
    free(d);
}

void bork_play_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 2);
    bork_grab_mouse(core, 1);
    struct bork_play_data* d = malloc(sizeof(*d));
    *d = (struct bork_play_data) {
        .core = core,
        .menu.state = BORK_MENU_CLOSED,
        .player_speed = 0.015,
        .held_item = -1,
        .quick_item = { -1, -1, -1, -1 },
    };
    bork_entity_init(&d->plr, BORK_ENTITY_PLAYER);
    /*  Initialize the player   */
    ARR_INIT(d->bullets);
    ARR_INIT(d->lights_buf);
    ARR_INIT(d->inventory);
    vec3_set(d->plr.pos, 32, 32, 40);
    d->plr.HP = 75;
    /*  Generate the BONZ station   */
    struct bork_editor_map ed_map = {};
    bork_editor_load_map(&ed_map, "test.bork_map");
    bork_editor_complete_map(&d->map, &ed_map);
    bork_map_init_model(&d->map, core);
    pg_shader_buffer_model(&d->core->shader_sprite, &d->map.door_model);
    d->map.plr = &d->plr,
    /*  Initialize the player data  */
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
    bork_poll_input(d->core);
    if(d->core->user_exit) state->running = 0;
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

/*  Update code */
static void tick_control_play(struct bork_play_data* d);
static void tick_control_inv_menu(struct bork_play_data* d);
static void tick_enemies(struct bork_play_data* d);
static void tick_items(struct bork_play_data* d);
static void tick_bullets(struct bork_play_data* d);

static void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    d->ticks = state->ticks;
    uint8_t* kmap = d->core->ctrl_map;
    /*  Handle input    */
    if(d->core->ctrl_state[kmap[BORK_CTRL_ESCAPE]] == BORK_CONTROL_HIT) {
        if(d->menu.state == BORK_MENU_INVENTORY) {
            d->menu.state = BORK_MENU_CLOSED;
            bork_grab_mouse(d->core, 1);
        } else {
            d->menu.state = BORK_MENU_INVENTORY;
            SDL_ShowCursor(SDL_ENABLE);
            bork_grab_mouse(d->core, 0);
        }
    }
    if(d->menu.state == BORK_MENU_CLOSED) tick_control_play(d);
    else if(d->menu.state == BORK_MENU_INVENTORY) tick_control_inv_menu(d);
    /*  Player update   */
    if(d->menu.state == BORK_MENU_CLOSED) {
        vec2_set(d->plr.dir,
                 d->plr.dir[0] + d->core->mouse_motion[0],
                 d->plr.dir[1] + d->core->mouse_motion[1]);
        d->plr.dir[0] = fmodf(d->plr.dir[0], M_PI * 2.0f);
        bork_entity_update(&d->plr, &d->map);
        /*  Everything else update  */
        bork_map_update(&d->map, &d->plr);
        tick_bullets(d);
        tick_enemies(d);
        tick_items(d);
        /*  Find the item the player is looking at, if any    */
        vec3 look_dir, look_pos;
        spherical_to_cartesian(look_dir, (vec2){ d->plr.dir[0] - M_PI,
                                                 d->plr.dir[1] - (M_PI * 0.5) });
        vec3_dup(look_pos, d->plr.pos);
        look_pos[2] += 0.8;
        int i;
        d->looked_item = -1;
        bork_entity_t ent_id;
        struct bork_entity* ent;
        float closest_angle = 0.2f, closest_dist = 2.5f;
        ARR_FOREACH(d->map.items, ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent) continue;
            ent->flags &= ~BORK_ENTFLAG_LOOKED_AT;
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, ent->pos, look_pos);
            float dist = vec3_len(ent_to_plr);
            if(dist >= closest_dist) continue;
            vec3_normalize(ent_to_plr, ent_to_plr);
            float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
            if(angle >= closest_angle) continue;
            closest_dist = dist;
            closest_angle = angle;
            d->looked_item = ent_id;
            d->looked_idx = i;
        }
        if(d->looked_item >= 0) {
            ent = bork_entity_get(d->looked_item);
            ent->flags |= BORK_ENTFLAG_LOOKED_AT;
        }
    }
    bork_ack_input(d->core);
}

static void tick_control_play(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    uint8_t* ctrl = d->core->ctrl_state;
    if(bork_input_event(d->core, kmap[BORK_CTRL_SELECT], BORK_CONTROL_HIT)
    && d->looked_item >= 0) {
        struct bork_entity* item = bork_entity_get(d->looked_item);
        if(item && bork_map_check_vis(&d->map, d->plr.pos, item->pos)) {
            item->flags &= ~BORK_ENTFLAG_LOOKED_AT;
            if(item->type == BORK_ITEM_BULLETS) {
                d->ammo_bullets += 30;
            } else {
                ARR_PUSH(d->inventory, d->looked_item);
            }
            ARR_SWAPSPLICE(d->map.items, d->looked_idx, 1);
        }
    }
    if(bork_input_event(d->core, kmap[BORK_CTRL_JUMP], BORK_CONTROL_HIT)
    && d->plr.flags & BORK_ENTFLAG_GROUND) {
        d->plr.vel[2] = 0.3;
        d->plr.flags &= ~BORK_ENTFLAG_GROUND;
        bork_entity_t new_id = bork_entity_new(1);
        struct bork_entity* new_ent = bork_entity_get(new_id);
        bork_entity_init(new_ent, (rand() % 2) ? BORK_ITEM_DOGFOOD : BORK_ITEM_MACHINEGUN);
        vec3_dup(new_ent->pos, d->plr.pos);
        ARR_PUSH(d->map.items, new_id);
    }
    float move_speed = d->player_speed * (d->plr.flags & BORK_ENTFLAG_GROUND ? 1 : 0.15);
    if(ctrl[kmap[BORK_CTRL_LEFT]]) {
        d->plr.vel[0] -= move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * cos(d->plr.dir[0]);
    }
    if(ctrl[kmap[BORK_CTRL_RIGHT]]) {
        d->plr.vel[0] += move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * cos(d->plr.dir[0]);
    }
    if(ctrl[kmap[BORK_CTRL_UP]]) {
        d->plr.vel[0] += move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * sin(d->plr.dir[0]);
    }
    if(ctrl[kmap[BORK_CTRL_DOWN]]) {
        d->plr.vel[0] -= move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * sin(d->plr.dir[0]);
    }
    if(ctrl[kmap[BORK_CTRL_FIRE]] && d->held_item >= 0) {
        bork_entity_t held_id = d->inventory.data[d->held_item];
        struct bork_entity* held_ent = bork_entity_get(held_id);
        if(held_ent) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[held_ent->type];
            if(prof->use_func && ctrl[kmap[BORK_CTRL_FIRE]] == prof->use_ctrl) {
                prof->use_func(held_ent, d);
            }
        }
    }
    if(ctrl[kmap[BORK_CTRL_BIND1]] == BORK_CONTROL_HIT) {
        d->held_item = d->quick_item[0];
    } else if(ctrl[kmap[BORK_CTRL_BIND2]] == BORK_CONTROL_HIT) {
        d->held_item = d->quick_item[1];
    } else if(ctrl[kmap[BORK_CTRL_BIND3]] == BORK_CONTROL_HIT) {
        d->held_item = d->quick_item[2];
    } else if(ctrl[kmap[BORK_CTRL_BIND4]] == BORK_CONTROL_HIT) {
        d->held_item = d->quick_item[3];
    }
}

static void tick_enemies(struct bork_play_data* d)
{
    int i;
    bork_entity_t ent_id;
    struct bork_entity* ent;
    ARR_FOREACH_REV(d->map.enemies, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent) {
            ARR_SWAPSPLICE(d->map.enemies, i, 1);
            continue;
        }
        vec3 ent_head, plr_head;
        vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
        vec3_add(plr_head, d->plr.pos, (vec3){ 0, 0, 0.5 });
        int vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis && ent->counter[0] < d->ticks) {
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, plr_head, ent_head);
            struct bork_bullet new_bullet = { .type = 1,
                .flags = BORK_BULLET_HURTS_PLAYER };
            vec3_set_len(new_bullet.dir, ent_to_plr, 1);
            vec3_add(new_bullet.pos, ent_head, new_bullet.dir);
            ARR_PUSH(d->bullets, new_bullet);
            ent->counter[0] = d->ticks + 60;
        }
        bork_entity_update(ent, &d->map);
    }
}

static void tick_items(struct bork_play_data* d)
{
    int i;
    bork_entity_t ent_id;
    struct bork_entity* ent;
    ARR_FOREACH_REV(d->map.items, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent) {
            ARR_SWAPSPLICE(d->map.items, i, 1);
            continue;
        }
        bork_entity_update(ent, &d->map);
    }
}

static void tick_bullets(struct bork_play_data* d)
{
    struct bork_bullet* blt;
    int i;
    ARR_FOREACH_PTR_REV(d->bullets, blt, i) {
        if(blt->flags & BORK_BULLET_DEAD) {
            if(blt->dead_ticks == 0) {
                ARR_SWAPSPLICE(d->bullets, i, 1);
                continue;
            }
            --blt->dead_ticks;
        } else {
            bork_bullet_move(blt, &d->map);
        }
    }
}

/*  Drawing */
static void draw_weapon(struct bork_play_data* d, vec3 pos_lerp, vec2 dir_lerp);
static void draw_enemies(struct bork_play_data* d, float lerp);
static void draw_items(struct bork_play_data* d, float lerp);
static void draw_bullets(struct bork_play_data* d, float lerp);
static void draw_map_lights(struct bork_play_data* d);
static void draw_light(struct bork_play_data* d, vec4 light, vec3 color);
static void draw_menu_inv(struct bork_play_data* d, float t);
static void draw_quickfetch_text(struct bork_play_data* d, int draw_label,
                                 vec4 color_mod, vec4 selected_mod);
static void draw_quickfetch_items(struct bork_play_data* d,
                                  vec4 color_mod, vec4 selected_mod);

static void bork_play_draw(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    /*  Interpolation   */
    float t = d->menu.state == BORK_MENU_CLOSED ? state->tick_over : 0;
    vec3 vel_lerp, draw_pos;
    vec2 draw_dir;
    vec3_scale(vel_lerp, d->plr.vel, t);
    vec3_add(draw_pos, d->plr.pos, vel_lerp);
    vec3_add(draw_pos, draw_pos, (vec3){ 0, 0, 0.8 });
    vec2_add(draw_dir, d->plr.dir, d->core->mouse_motion);
    pg_viewer_set(&d->core->view, draw_pos, draw_dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->core->gbuf);
    pg_shader_begin(&d->core->shader_3d, &d->core->view);
    draw_weapon(d, draw_pos, draw_dir);
    pg_shader_begin(&d->core->shader_sprite, &d->core->view);
    draw_enemies(d, t);
    draw_items(d, t);
    draw_bullets(d, t);
    pg_shader_3d_texture(&d->core->shader_3d, &d->core->env_atlas);
    bork_map_draw(&d->map, d->core);
    draw_map_lights(d);
    /*  Lighting    */
    pg_gbuffer_begin_light(&d->core->gbuf, &d->core->view);
    int i;
    struct bork_light* light;
    ARR_FOREACH_PTR(d->lights_buf, light, i) {
        pg_gbuffer_draw_light(&d->core->gbuf, light->pos, light->color);
    }
    pg_gbuffer_begin_spotlight(&d->core->gbuf, &d->core->view);
    ARR_FOREACH_PTR(d->spotlights, light, i) {
        pg_gbuffer_draw_spotlight(&d->core->gbuf, light->pos, light->dir_angle, light->color);
    }
    ARR_TRUNCATE(d->lights_buf, 0);
    ARR_TRUNCATE(d->spotlights, 0);
    if(d->menu.state == BORK_MENU_CLOSED) pg_screen_dst();
    else pg_ppbuffer_dst(&d->core->ppbuf);
    pg_gbuffer_finish(&d->core->gbuf, (vec3){ 0.05, 0.05, 0.05 });
    if(d->menu.state != BORK_MENU_CLOSED) {
        pg_postproc_blur_scale(&d->core->post_blur, (vec2){ 3, 3 });
        pg_ppbuffer_swapdst(&d->core->ppbuf);
        pg_postproc_blur_dir(&d->core->post_blur, 1);
        pg_postproc_apply(&d->core->post_blur, &d->core->ppbuf);
        pg_postproc_blur_dir(&d->core->post_blur, 0);
        pg_ppbuffer_swap(&d->core->ppbuf);
        pg_screen_dst();
        pg_postproc_apply(&d->core->post_blur, &d->core->ppbuf);
        pg_shader_begin(&d->core->shader_2d, NULL);
        draw_menu_inv(d, (float)state->ticks / (float)state->tick_rate);
    } else {
        /*  Overlay */
        pg_shader_begin(&d->core->shader_text, NULL);
        if(d->held_item >= 0) {
            bork_entity_t held_id = d->inventory.data[d->held_item];
            struct bork_entity* held_ent = bork_entity_get(held_id);
            if(held_ent) {
                const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[held_ent->type];
                if(prof->hud_func) prof->hud_func(held_ent, d);
            }
        }
        draw_quickfetch_text(d, 0, (vec4){ 1, 1, 1, 0.15 }, (vec4){ 1, 1, 1, 0.75 });
        draw_quickfetch_items(d, (vec4){ 1, 1, 1, 0.15 }, (vec4){ 1, 1, 1, 0.75 });
        pg_shader_begin(&d->core->shader_2d, NULL);
        pg_model_begin(&d->core->quad_2d, &d->core->shader_2d);
        pg_shader_2d_resolution(&d->core->shader_2d, (vec2){ d->core->aspect_ratio, 1 });
        pg_shader_2d_tex_frame(&d->core->shader_2d, 60);
        pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ 4, 1 }, (vec2){ 0, 0 });
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.1, 0.8 }, (vec2){ 0.4, 0.1 }, 0);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
        pg_model_draw(&d->core->quad_2d, NULL);
        float hp_frac = (float)d->plr.HP / 100.0f;
        pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ hp_frac, 1 }, (vec2){ -0.5, 0 });
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.1, 0.8 }, (vec2){ 0.4 * hp_frac, 0.1 }, 0);
        pg_model_draw(&d->core->quad_2d, NULL);
    }
    pg_shader_begin(&d->core->shader_text, NULL);
    bork_draw_fps(d->core);
}

static void draw_weapon(struct bork_play_data* d, vec3 pos_lerp, vec2 dir_lerp)
{
    struct pg_shader* shader = &d->core->shader_3d;
    struct pg_model* model = &d->core->gun_model;
    struct bork_entity* held_item;
    const struct bork_entity_profile* prof;
    if(d->held_item < 0
    || !(held_item = bork_entity_get(d->inventory.data[d->held_item])))
        return;
    prof = &BORK_ENT_PROFILES[held_item->type];
    pg_shader_3d_texture(shader, &d->core->item_tex);
    pg_shader_3d_tex_frame(shader, prof->front_frame);
    mat4 tx;
    mat4_identity(tx);
    mat4_translate(tx, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
    mat4_rotate_Z(tx, tx, M_PI + dir_lerp[0]);
    mat4_rotate_Y(tx, tx, -dir_lerp[1]);
    pg_shader_3d_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
    mat4 offset;
    mat4_translate(offset, -0.6, 0.3, -0.2);
    mat4_mul(tx, tx, offset);
    mat4_scale_aniso(tx, tx, prof->sprite_tx[0], prof->sprite_tx[1], 1);
    pg_model_begin(model, shader);
    pg_model_draw(model, tx);
}

static void draw_enemies(struct bork_play_data* d, float lerp)
{
    struct bork_map* map = &d->map;
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->enemy_model;
    vec3 plr_pos;
    vec3_dup(plr_pos, d->plr.pos);
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    pg_shader_sprite_texture(shader, &d->core->env_atlas);
    pg_model_begin(model, shader);
    int current_frame = 0;
    int i;
    struct bork_entity* ent;
    const struct bork_entity_profile* prof;
    bork_entity_t ent_id;
    ARR_FOREACH(map->enemies, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent || (ent->flags & BORK_ENTFLAG_DEAD)) continue;
        prof = &BORK_ENT_PROFILES[ent->type];
        /*  Figure out which directional frame to draw  */
        vec3 pos_lerp;
        vec3_scale(pos_lerp, ent->vel, lerp);
        vec3_add(pos_lerp, pos_lerp, ent->pos);
        vec2 ent_to_plr;
        vec2_sub(ent_to_plr, pos_lerp, plr_pos);
        float dir_adjust = 1.0f / (float)prof->dir_frames + 0.5f;
        float angle = atan2(ent_to_plr[0], ent_to_plr[1]) + (M_PI * dir_adjust) + ent->dir[0];
        if(angle < 0) angle += M_PI * 2;
        else if(angle > (M_PI * 2)) angle = fmodf(angle, M_PI * 2);
        float angle_f = angle / (M_PI * 2);
        int frame = (int)(angle_f * (float)prof->dir_frames) + prof->front_frame;
        if(frame != current_frame) {
            pg_shader_sprite_tex_frame(shader, frame);
            current_frame = frame;
        }
        /*  Draw the sprite */
        mat4 transform;
        mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
        pg_model_draw(model, transform);
    }
}

static void draw_items(struct bork_play_data* d, float lerp)
{
    struct bork_map* map = &d->map;
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->bullet_model;
    int current_frame = 0;
    pg_shader_sprite_mode(shader, PG_SPRITE_SPHERICAL);
    pg_shader_sprite_texture(shader, &d->core->item_tex);
    pg_shader_sprite_tex_frame(shader, 0);
    pg_shader_sprite_add_tex_tx(shader, (vec2){ 1, 1 }, (vec2){});
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){});
    pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(model, shader);
    int i;
    struct bork_entity* ent;
    const struct bork_entity_profile* prof;
    bork_entity_t ent_id;
    ARR_FOREACH(map->items, ent_id, i) {
        ent = bork_entity_get(ent_id);
        prof = &BORK_ENT_PROFILES[ent->type];
        if(ent->flags & BORK_ENTFLAG_DEAD) continue;
        if(prof->front_frame != current_frame) {
            current_frame = prof->front_frame;
            pg_shader_sprite_tex_frame(shader, prof->front_frame);
            pg_shader_sprite_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
            pg_shader_sprite_transform(shader, prof->sprite_tx, prof->sprite_tx + 2);
        }
        vec3 pos_lerp;
        vec3_scale(pos_lerp, ent->vel, lerp);
        vec3_add(pos_lerp, pos_lerp, ent->pos);
        mat4 transform;
        mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
        mat4_scale(transform, transform, 0.5);
        if(ent->flags & BORK_ENTFLAG_LOOKED_AT) {
            pg_shader_sprite_color_mod(shader, (vec4){ 1.5f, 1.8f, 1.5f, 1.0f });
            pg_model_draw(model, transform);
            pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
        } else pg_model_draw(model, transform);
    }
}

static void draw_bullets(struct bork_play_data* d, float lerp)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->bullet_model;
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    pg_shader_sprite_texture(shader, &d->core->bullet_tex);
    pg_shader_sprite_tex_frame(shader, 0);
    pg_shader_sprite_mode(shader, PG_SPRITE_SPHERICAL);
    pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(model, shader);
    mat4 bullet_transform;
    int current_frame = 0;
    struct bork_bullet* blt;
    int i;
    ARR_FOREACH_PTR(d->bullets, blt, i) {
        if(blt->flags & BORK_BULLET_DEAD) {
            draw_light(d, (vec4){ blt->pos[0], blt->pos[1], blt->pos[2],
                                  ((float)blt->dead_ticks / 10.0f) * 3.0f },
                          blt->light_color);
            continue;
        }
        if(blt->type != current_frame) {
            pg_shader_sprite_tex_frame(shader, blt->type);
            current_frame = blt->type;
        }
        vec3 pos_lerp;
        vec3_scale(pos_lerp, blt->dir, lerp);
        vec3_add(pos_lerp, pos_lerp, blt->pos);
        mat4_translate(bullet_transform, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
        pg_model_draw(model, bullet_transform);
    }
}

static void draw_map_lights(struct bork_play_data* d)
{
    int i;
    struct bork_light* light;
    ARR_FOREACH_PTR(d->map.lights, light, i) {
        /*
        vec3 light_to_plr;
        vec3_sub(light_to_plr, light->pos, d->plr.pos);
        float dist = vec3_len(light_to_plr);
        if(dist > 50) continue;
        else if(dist > 30) light->pos[3] *= 1 - (dist - 30) / 20;*/
        ARR_PUSH(d->lights_buf, *light);
    }
    ARR_FOREACH_PTR(d->map.spotlights, light, i) {
        /*
        vec3 light_to_plr;
        vec3_sub(light_to_plr, light->pos, d->plr.pos);
        float dist = vec3_len(light_to_plr);
        if(dist > 50) continue;
        else if(dist > 30) light->pos[3] *= 1 - (dist - 30) / 20;*/
        ARR_PUSH(d->spotlights, *light);
    }
}

static void draw_light(struct bork_play_data* d, vec4 light, vec3 color)
{
    struct bork_light new_light = {
        .pos = { light[0], light[1], light[2], light[3] },
        .color = { color[0], color[1], color[2] } };
    ARR_PUSH(d->lights_buf, new_light);
}

static void tick_control_inv_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(bork_input_event(d->core, kmap[BORK_CTRL_DOWN], BORK_CONTROL_HIT)) {
        d->menu.selection_idx = MIN(d->menu.selection_idx + 1, d->inventory.len - 1);
        if(d->menu.selection_idx >= d->menu.scroll_idx + 10) ++d->menu.scroll_idx;
    }
    if(bork_input_event(d->core, kmap[BORK_CTRL_UP], BORK_CONTROL_HIT)) {
        d->menu.selection_idx = MAX(d->menu.selection_idx - 1, 0);
        if(d->menu.selection_idx < d->menu.scroll_idx) --d->menu.scroll_idx;
    }
    if(bork_input_event(d->core, kmap[BORK_CTRL_BIND1], BORK_CONTROL_HIT)) {
        d->quick_item[0] = d->menu.selection_idx;
    }
    if(bork_input_event(d->core, kmap[BORK_CTRL_BIND2], BORK_CONTROL_HIT)) {
        d->quick_item[1] = d->menu.selection_idx;
    }
    if(bork_input_event(d->core, kmap[BORK_CTRL_BIND3], BORK_CONTROL_HIT)) {
        d->quick_item[2] = d->menu.selection_idx;
    }
    if(bork_input_event(d->core, kmap[BORK_CTRL_BIND4], BORK_CONTROL_HIT)) {
        d->quick_item[3] = d->menu.selection_idx;
    }

}

static void draw_quickfetch_items(struct bork_play_data* d,
                                  vec4 color_mod, vec4 selected_mod)
{
    struct pg_shader* shader = &d->core->shader_2d;
    float w = d->core->aspect_ratio;
    pg_shader_2d_resolution(shader, (vec2){ w, 1});
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_tex_frame(shader, 0);
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    struct bork_entity* item;
    const struct bork_entity_profile* prof;
    int current_frame = 0;
    int i;
    vec2 draw_pos[4] = {
        { w - 0.2 + 0.015, 0.75 }, { w - 0.25 + 0.015, 0.8 },
        { w - 0.15 + 0.015, 0.8 }, { w - 0.2 + 0.015, 0.85 } };
    for(i = 0; i < 4; ++i) {
        if(d->quick_item[i] < 0) continue;
        item = bork_entity_get(d->inventory.data[d->quick_item[i]]);
        if(!item) continue;
        prof = &BORK_ENT_PROFILES[item->type];
        if(prof->front_frame != current_frame) {
            current_frame = prof->front_frame;
            pg_shader_2d_tex_frame(shader, prof->front_frame);
            pg_shader_2d_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
        }
        if(d->held_item != d->quick_item[i]) {
            pg_shader_2d_color_mod(shader, color_mod);
        } else {
            pg_shader_2d_color_mod(shader, selected_mod);
        }
        pg_shader_2d_transform(shader, draw_pos[i],
            (vec2){ 0.05 * prof->sprite_tx[0], 0.05 * prof->sprite_tx[1] }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
}

static void draw_quickfetch_text(struct bork_play_data* d, int draw_label,
                                 vec4 color_mod, vec4 selected_mod)
{
    struct pg_shader* shader = &d->core->shader_text;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    float w = d->core->aspect_ratio;
    pg_shader_text_resolution(shader, (vec2){ w, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = {
        .use_blocks = draw_label ? 6 : 4,
        .block = {
            "1", "2", "3", "4",
            "QUICK", "FETCH",
        },
        .block_style = {
            { w - 0.2, 0.8, 0.03, 1 }, { w - 0.25, 0.85, 0.03, 1 },
            { w - 0.15, 0.85, 0.03, 1 }, { w - 0.2, 0.9, 0.03, 1 },
            { w - 0.45, 0.83, 0.025, 1.25 },
            { w - 0.45, 0.88, 0.025, 1.25 },
        },
        .block_color = {
            { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 },
            { 1, 1, 1, 1 }, { 1, 1, 1, 1 },
        },
    };
    int i;
    for(i = 0; i < 4; ++i) {
        if(d->held_item >= 0 && d->quick_item[i] == d->held_item)
            vec4_mul(text.block_color[i], text.block_color[i], selected_mod);
        else
            vec4_mul(text.block_color[i], text.block_color[i], color_mod);
    }
    pg_shader_text_write(&d->core->shader_text, &text);
};

static void draw_inventory_text(struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_text;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    float w = d->core->aspect_ratio;
    pg_shader_2d_resolution(shader, (vec2){ w, 1 });
    int inv_len = MIN(10, d->inventory.len);
    int inv_start = d->menu.scroll_idx;
    struct pg_shader_text text = {
        .use_blocks = inv_len + 1,
        .block = {
            "INVENTORY",
        },
        .block_style = {
            { 0.1, 0.1, 0.05, 1.25 },
        },
        .block_color = {
            { 1, 1, 1, 0.7 },
        },
    };
    int i;
    int ti = 1;
    for(i = 0; i < inv_len; ++i, ++ti) {
        struct bork_entity* item = bork_entity_get(d->inventory.data[i + inv_start]);
        if(!item) continue;
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
        strncpy(text.block[ti], prof->name, 64);
        vec4_set(text.block_style[ti], 0.1, 0.2 + 0.06 * i, 0.04, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.5);
        if(i + inv_start == d->menu.selection_idx) {
            text.block_style[ti][0] += 0.05;
            text.block_color[ti][3] = 0.75;
        }
    }
    pg_shader_text_write(&d->core->shader_text, &text);
}

static void draw_menu_inv(struct bork_play_data* d, float t)
{
    struct pg_shader* shader = &d->core->shader_2d;
    bork_draw_backdrop(d->core, (vec4){ 0.7, 0.7, 0.7, 0.5 }, t);
    bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 0.75 });
    shader = &d->core->shader_text;
    pg_shader_begin(&d->core->shader_text, NULL);
    pg_shader_text_resolution(shader, (vec2){d->core->aspect_ratio, 1});
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    draw_quickfetch_text(d, 1, (vec4){ 1, 1, 1, 0.75 }, (vec4){ 1, 1, 1, 0.9 });
    draw_inventory_text(d);
    draw_quickfetch_items(d, (vec4){ 1, 1, 1, 0.75 }, (vec4){ 1, 1, 1, 0.9 });
}

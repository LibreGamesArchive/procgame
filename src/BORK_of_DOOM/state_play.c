#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
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

struct bork_light {
    vec4 pos;
    vec3 color;
};

struct bork_play_data {
    struct bork_game_core* core;
    ARR_T(struct bork_entity) ent_pool;
    struct bork_map map;
    enum bork_area current_area;
    struct bork_entity plr;
    float player_speed;
    const uint8_t* kb_state;
    vec2 mouse_motion;
    struct {
        enum {
            BORK_MENU_CLOSED,
            BORK_MENU_INVENTORY,
            BORK_MENU_CHARACTER,
        } state;
        int selection_idx;
    } menu;
    struct bork_entity* looked_item;
    int looked_idx;
    ARR_T(struct bork_entity) inventory;
    ARR_T(struct bork_bullet) bullets;
    ARR_T(struct bork_light) lights_buf;    /*  Updated per frame   */
};

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
    struct bork_play_data* d = malloc(sizeof(*d));
    d->core = core;
    d->kb_state = SDL_GetKeyboardState(NULL);
    d->menu.state = BORK_MENU_CLOSED;
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
    d->player_speed = 0.02;
    d->map.plr = &d->plr;
    d->current_area = 0;
    ARR_INIT(d->bullets);
    ARR_INIT(d->lights_buf);
    ARR_INIT(d->inventory);
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
    if(d->menu.state == BORK_MENU_CLOSED) {
        d->mouse_motion[0] -= mx * 0.0005f;
        d->mouse_motion[1] -= my * 0.0005f;
    }
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
static void tick_enemies(struct bork_play_data* d);
static void tick_items(struct bork_play_data* d);
static void tick_bullets(struct bork_play_data* d);

static void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    /*  Handle input    */
    if(d->core->ctrl_state[BORK_CTRL_ESCAPE] == BORK_CONTROL_HIT) {
        if(d->menu.state == BORK_MENU_INVENTORY) {
            d->menu.state = BORK_MENU_CLOSED;
        } else {
            d->menu.state = BORK_MENU_INVENTORY;
        }
    }
    if(d->menu.state == BORK_MENU_CLOSED) tick_control_play(d);
    bork_update_inputs(d->core);
    /*  Player update   */
    if(d->menu.state == BORK_MENU_CLOSED) {
        vec2_set(d->plr.dir,
                 d->plr.dir[0] + d->mouse_motion[0],
                 d->plr.dir[1] + d->mouse_motion[1]);
        d->plr.dir[0] = fmodf(d->plr.dir[0], M_PI * 2.0f);
        vec2_set(d->mouse_motion, 0, 0);
        bork_entity_update(&d->plr, &d->map);
        /*  Everything else update  */
        bork_map_update_area(&d->map, 0, &d->plr);
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
        d->looked_item = NULL;
        struct bork_entity* ent;
        float closest_angle = 0.2f, closest_dist = 2.5f;
        ARR_FOREACH_PTR(d->map.items[d->current_area], ent, i) {
            ent->item.looked_at = 0;
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, ent->pos, look_pos);
            float dist = vec3_len(ent_to_plr);
            if(dist >= closest_dist) continue;
            vec3_normalize(ent_to_plr, ent_to_plr);
            float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
            if(angle >= closest_angle) continue;
            closest_dist = dist;
            closest_angle = angle;
            d->looked_item = ent;
            d->looked_idx = i;
        }
        if(d->looked_item) {
            d->looked_item->item.looked_at = 1;
        }
    }
}

static void tick_control_play(struct bork_play_data* d)
{
    if(d->core->ctrl_state[BORK_CTRL_SELECT] == BORK_CONTROL_HIT && d->looked_item) {
        d->looked_item->item.looked_at = 0;
        ARR_PUSH(d->inventory, *d->looked_item);
        ARR_SWAPSPLICE(d->map.items[d->current_area], d->looked_idx, 1);
    }
    if(d->core->ctrl_state[BORK_CTRL_JUMP] == BORK_CONTROL_HIT && d->plr.ground) {
        d->plr.vel[2] = 0.3;
        d->plr.ground = 0;
    }
    float move_speed = d->player_speed * (d->plr.ground ? 1 : 0.15);
    if(d->core->ctrl_state[BORK_CTRL_LEFT]) {
        d->plr.vel[0] -= move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * cos(d->plr.dir[0]);
    }
    if(d->core->ctrl_state[BORK_CTRL_RIGHT]) {
        d->plr.vel[0] += move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * cos(d->plr.dir[0]);
    }
    if(d->core->ctrl_state[BORK_CTRL_UP]) {
        d->plr.vel[0] += move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * sin(d->plr.dir[0]);
    }
    if(d->core->ctrl_state[BORK_CTRL_DOWN]) {
        d->plr.vel[0] -= move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * sin(d->plr.dir[0]);
    }
    if(d->core->ctrl_state[BORK_CTRL_FIRE] == BORK_CONTROL_HIT) {
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
    if(d->core->ctrl_state[BORK_CTRL_BIND1] == BORK_CONTROL_HIT) {
        vec3 item_dir;
        spherical_to_cartesian(item_dir, (vec2){ d->plr.dir[0] - M_PI,
                                                 d->plr.dir[1] - (M_PI * 0.5) });
        vec3_scale(item_dir, item_dir, 1);
        struct bork_entity new_item = {
            .active = 1, .type = BORK_ENTITY_ITEM,
            .size = { 0.5, 0.5, 0.5 } };
        vec3_dup(new_item.pos, d->plr.pos);
        vec3_dup(new_item.vel, item_dir);
        new_item.pos[0] += 0.2 * sin(d->plr.dir[0]);
        new_item.pos[1] -= 0.2 * cos(d->plr.dir[0]);
        new_item.pos[2] += 0.75;
        vec3_scale(new_item.vel, new_item.vel, 0.2);
        ARR_PUSH(d->map.items[0], new_item);
    }
    if(d->core->ctrl_state[BORK_CTRL_BIND2] == BORK_CONTROL_HIT) {
         struct bork_entity new_ent = { .type = BORK_ENTITY_ENEMY };
         vec3_dup(new_ent.pos, d->plr.pos);
         vec3_set(new_ent.size, 1, 1, 1);
         vec2_set(new_ent.dir, d->plr.dir[0], 0);
         new_ent.dir_frames = 4;
         new_ent.front_frame = 192;
         ARR_PUSH(d->map.enemies[d->current_area], new_ent);
    }
}

static void tick_enemies(struct bork_play_data* d)
{
    int i;
    struct bork_entity* ent;
    ARR_FOREACH_PTR_REV(d->map.enemies[d->current_area], ent, i) {
        if(ent->dead) {
            ARR_SWAPSPLICE(d->map.enemies[d->current_area], i, 1);
            continue;
        }
        bork_entity_update(ent, &d->map);
    }
}

static void tick_items(struct bork_play_data* d)
{
    int i;
    struct bork_entity* ent;
    ARR_FOREACH_PTR_REV(d->map.items[d->current_area], ent, i) {
        if(ent->dead) {
            ARR_SWAPSPLICE(d->map.items[d->current_area], i, 1);
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
        if(blt->dead_ticks == 1) {
            ARR_SWAPSPLICE(d->bullets, i, 1);
            continue;
        } else if(blt->dead_ticks) --blt->dead_ticks;
        else bork_bullet_move(blt, &d->map);
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
    vec2_add(draw_dir, d->plr.dir, d->mouse_motion);
    pg_viewer_set(&d->core->view, draw_pos, draw_dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->core->gbuf);
    pg_shader_begin(&d->core->shader_3d, &d->core->view);
    draw_weapon(d, draw_pos, draw_dir);
    pg_shader_begin(&d->core->shader_sprite, &d->core->view);
    draw_enemies(d, t);
    draw_items(d, t);
    draw_bullets(d, t);
    pg_shader_3d_set_texture(&d->core->shader_3d, &d->core->env_atlas);
    bork_map_draw_area(&d->map, d->current_area);
    draw_map_lights(d);
    /*  Lighting    */
    pg_gbuffer_begin_light(&d->core->gbuf, &d->core->view);
    int i;
    struct bork_light* light;
    ARR_FOREACH_PTR(d->lights_buf, light, i) {
        pg_gbuffer_draw_light(&d->core->gbuf, light->pos, light->color);
    }
    ARR_TRUNCATE(d->lights_buf, 0);
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
    }
    /*  Overlay */
    pg_shader_text_resolution(&d->core->shader_text, d->core->screen_size);
    pg_shader_begin(&d->core->shader_text, NULL);
    bork_draw_fps(d->core);
}

static void draw_weapon(struct bork_play_data* d, vec3 pos_lerp, vec2 dir_lerp)
{
    struct pg_shader* shader = &d->core->shader_3d;
    struct pg_model* model = &d->core->gun_model;
    pg_shader_3d_set_texture(shader, &d->core->item_tex);
    pg_shader_3d_set_tex_frame(shader, 8);
    mat4 tx;
    mat4_identity(tx);
    mat4_translate(tx, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
    mat4_rotate_Z(tx, tx, M_PI + dir_lerp[0]);
    mat4_rotate_Y(tx, tx, -dir_lerp[1]);
    mat4 offset;
    mat4_translate(offset, -0.6, 0.3, -0.2);
    mat4_mul(tx, tx, offset);
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
    pg_shader_sprite_set_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_set_texture(shader, &map->core->env_atlas);
    pg_model_begin(model, shader);
    int current_frame = 0;
    int i;
    struct bork_entity* ent;
    ARR_FOREACH_PTR(map->enemies[d->current_area], ent, i) {
        if(ent->dead) continue;
        /*  Figure out which directional frame to draw  */
        vec3 pos_lerp;
        vec3_scale(pos_lerp, ent->vel, lerp);
        vec3_add(pos_lerp, pos_lerp, ent->pos);
        vec2 ent_to_plr;
        vec2_sub(ent_to_plr, pos_lerp, plr_pos);
        float dir_adjust = 1.0f / (float)ent->dir_frames + 0.5f;
        float angle = atan2(ent_to_plr[0], ent_to_plr[1]) + (M_PI * dir_adjust) + ent->dir[0];
        if(angle < 0) angle += M_PI * 2;
        else if(angle > (M_PI * 2)) angle = fmodf(angle, M_PI * 2);
        float angle_f = angle / (M_PI * 2);
        int frame = (int)(angle_f * (float)ent->dir_frames) + ent->front_frame;
        if(frame != current_frame) {
            pg_shader_sprite_set_tex_frame(shader, frame);
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
    pg_shader_sprite_set_mode(shader, PG_SPRITE_SPHERICAL);
    pg_shader_sprite_set_texture(shader, &d->core->item_tex);
    pg_shader_sprite_set_tex_frame(shader, 0);
    pg_shader_sprite_set_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(model, shader);
    int i;
    struct bork_entity* ent;
    ARR_FOREACH_PTR(map->items[d->current_area], ent, i) {
        if(ent->dead) continue;
        if(ent->front_frame != current_frame) {
            pg_shader_sprite_set_tex_frame(shader, ent->front_frame);
            current_frame = ent->front_frame;
        }
        vec3 pos_lerp;
        vec3_scale(pos_lerp, ent->vel, lerp);
        vec3_add(pos_lerp, pos_lerp, ent->pos);
        mat4 transform;
        mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
        mat4_scale(transform, transform, 0.5);
        if(ent->item.looked_at) {
            pg_shader_sprite_set_color_mod(shader, (vec4){ 1.5f, 1.8f, 1.5f, 1.0f });
            pg_model_draw(model, transform);
            pg_shader_sprite_set_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
        } else pg_model_draw(model, transform);
    }
}

static void draw_bullets(struct bork_play_data* d, float lerp)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->bullet_model;
    pg_shader_sprite_set_texture(shader, &d->core->bullet_tex);
    pg_shader_sprite_set_tex_frame(shader, 0);
    pg_shader_sprite_set_mode(shader, PG_SPRITE_SPHERICAL);
    pg_shader_sprite_set_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(model, shader);
    mat4 bullet_transform;
    struct bork_bullet* blt;
    int i;
    ARR_FOREACH_PTR(d->bullets, blt, i) {
        if(blt->dead_ticks) {
            draw_light(d, (vec4){ blt->pos[0], blt->pos[1], blt->pos[2],
                                  ((float)blt->dead_ticks / 10.0f) * 3.0f },
                          blt->light_color);
            continue;
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
    struct bork_map_object* obj;
    ARR_FOREACH_PTR(d->map.objects[d->current_area], obj, i) {
        vec4 light = { obj->x * 2.0f + 1.0f,
                       obj->y * 2.0f + 1.0f,
                       obj->z * 2.0f + 1.5f, obj->light[3] };
        vec3 light_to_plr;
        vec3_sub(light_to_plr, light, d->plr.pos);
        float dist = vec3_len(light_to_plr);
        if(dist > 40) continue;
        else if(dist > 20) light[3] *= 1 - (dist - 20) / 20;
        draw_light(d, light, obj->light);
    }
}

static void draw_light(struct bork_play_data* d, vec4 light, vec3 color)
{
    struct bork_light new_light = {
        .pos = { light[0], light[1], light[2], light[3] },
        .color = { color[0], color[1], color[2] } };
    ARR_PUSH(d->lights_buf, new_light);
}

static void draw_menu_inv(struct bork_play_data* d, float t)
{
    struct pg_shader* shader = &d->core->shader_2d;
    bork_draw_backdrop(d->core, (vec4){ 1, 1, 1, 0.5 }, t);
    bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 0.8 });
    shader = &d->core->shader_text;
    pg_shader_text_resolution(shader, (vec2){ 1, 1 });
    float font_ratio = d->core->font.frame_aspect_ratio / d->core->aspect_ratio;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_text_transform(shader,
        (vec2){ 0.1, 0.1 }, (vec2){ font_ratio * 0.05, 0.05 });
    struct pg_shader_text text = { .use_blocks = 12 };
    int i;
    for(i = 0; i < 11; ++i) {
        strncpy(text.block[i], "BORK!?", 64);
        vec4_set(text.block_style[i], 2, 2 + 1.3 * i, 1, 1.2);
        vec4_set(text.block_color[i], 1, 1, 1, 0.5);
    }
    strncpy(text.block[i], "INVENTORY", 64);
    vec4_set(text.block_style[i], 0, 0, 1.5, 1.2);
    vec4_set(text.block_color[i], 1, 1, 1, 0.5);
    pg_shader_text_write(shader, &text);

}

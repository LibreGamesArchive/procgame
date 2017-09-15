#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "particle.h"
#include "entity.h"
#include "map_area.h"
#include "bullet.h"
#include "physics.h"
#include "game_states.h"
#include "datapad_content.h"

#define RANDF   ((float)rand() / RAND_MAX)

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
    ARR_DEINIT(d->lights_buf);
    ARR_DEINIT(d->spotlights);
    ARR_DEINIT(d->inventory);
    ARR_DEINIT(d->bullets);
    bork_entpool_clear();
    free(d);
}

void bork_play_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 120, 3);
    pg_mouse_mode(1);
    struct bork_play_data* d = malloc(sizeof(*d));
    *d = (struct bork_play_data) {
        .core = core,
        .menu.state = BORK_MENU_CLOSED,
        .player_speed = 0.008,
        .hud_datapad_id = -1,
        .looked_item = -1,
        .held_item = -1,
        .quick_item = { -1, -1, -1, -1 },
    };
    bork_entity_init(&d->plr, BORK_ENTITY_PLAYER);
    /*  Initialize the player   */
    ARR_INIT(d->bullets);
    ARR_INIT(d->lights_buf);
    ARR_INIT(d->inventory);
    vec3_set(d->plr.pos, 32, 32, 40);
    d->plr.HP = 100;
    /*  Generate the BONZ station   */
    struct bork_editor_map ed_map = {};
    bork_editor_load_map(&ed_map, "test.bork_map");
    bork_map_init(&d->map);
    d->map.plr = &d->plr;
    bork_editor_complete_map(&d->map, &ed_map);
    bork_map_init_model(&d->map, core);
    pg_shader_buffer_model(&d->core->shader_sprite, &d->map.door_model);
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
    pg_poll_input();
    if(d->menu.state == BORK_MENU_CLOSED) {
        vec2 mouse_motion;
        pg_mouse_motion(mouse_motion);
        vec2_scale(mouse_motion, mouse_motion, d->core->mouse_sensitivity);
        vec2_sub(d->mouse_motion, d->mouse_motion, mouse_motion);
        if(pg_have_gamepad()) {
            vec2 stick;
            pg_gamepad_stick(1, stick);
            vec2_scale(stick, stick, 0.05);
            vec2_sub(d->mouse_motion, d->mouse_motion, stick);
        }
    }
    if(pg_user_exit()) state->running = 0;
}

/*  Gameplay functions  */
void bork_play_reset_hud_anim(struct bork_play_data* d)
{
    struct bork_entity* held_item;
    const struct bork_entity_profile* prof;
    d->reload_ticks = 0;
    d->reload_length = 0;
    d->hud_anim_progress = 0;
    d->hud_anim_speed = 0;
    d->hud_anim_active = 0;
    d->hud_anim_destroy_when_finished = 0;
    d->hud_callback_frame = -1;
    d->hud_anim_callback = NULL;
    vec3_set(d->hud_anim[0], 0, 0, 0);
    vec3_set(d->hud_anim[1], 0, 0, 0);
    vec3_set(d->hud_anim[2], 0, 0, 0);
    vec3_set(d->hud_anim[3], 0, 0, 0);
    vec3_set(d->hud_anim[4], 0, 0, 0);
    d->hud_angle[1] = 0;
    d->hud_angle[2] = 0;
    d->hud_angle[3] = 0;
    d->hud_angle[4] = 0;
    if(d->held_item >= 0
    && (held_item = bork_entity_get(d->inventory.data[d->held_item]))) {
        prof = &BORK_ENT_PROFILES[held_item->type];
        d->hud_angle[0] = prof->hud_angle;
    } else d->hud_angle[0] = 0;
}

static bork_entity_t remove_inventory_item(struct bork_play_data* d, int inv_idx)
{
    if(inv_idx >= d->inventory.len) return -1;
    bork_entity_t item = d->inventory.data[inv_idx];
    ARR_SWAPSPLICE(d->inventory, inv_idx, 1);
    int i;
    for(i = 0; i < 4; ++i) {
        if(d->quick_item[i] == inv_idx) d->quick_item[i] = -1;
        else if(d->quick_item[i] > inv_idx) --d->quick_item[i];
    }
    if(d->held_item == inv_idx) d->held_item = -1;
    return item;
}

static void add_inventory_item(struct bork_play_data* d, bork_entity_t ent_id)
{
    struct bork_entity* ent = bork_entity_get(ent_id);
    if(!ent) return;
    if(ent->flags & BORK_ENTFLAG_STACKS) {
        struct bork_entity* inv_ent;
        bork_entity_t inv_id;
        int i;
        ARR_FOREACH(d->inventory, inv_id, i) {
            inv_ent = bork_entity_get(inv_id);
            if(!inv_ent || inv_ent->type != ent->type) continue;
            ++inv_ent->item_quantity;
            ent->flags |= BORK_ENTFLAG_DEAD;
            return;
        }
    }
    int inv_idx = d->inventory.len;
    ARR_PUSH(d->inventory, ent_id);
    if(d->held_item < 0) {
        d->held_item = inv_idx;
        bork_play_reset_hud_anim(d);
    }
    int i;
    for(i = 0; i < 4; ++i) {
        if(d->quick_item[i] < 0) {
            d->quick_item[i] = inv_idx;
            break;
        }
    }
}

static void switch_item(struct bork_play_data* d, int inv_idx)
{
    if(inv_idx < 0 || inv_idx >= d->inventory.len || d->held_item == inv_idx) return;
    if(d->hud_anim_active && d->hud_anim_destroy_when_finished) {
        struct bork_entity* ent = bork_entity_get(remove_inventory_item(d, d->held_item));
        ent->flags |= BORK_ENTFLAG_DEAD;
    }
    struct bork_entity* ent = bork_entity_get(d->inventory.data[inv_idx]);
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    d->held_item = inv_idx;
    bork_play_reset_hud_anim(d);
    d->hud_angle[0] = prof->hud_angle;
}

static void set_quick_item(struct bork_play_data* d, int quick_idx, int inv_idx)
{
    int i;
    for(i = 0; i < 4; ++i) {
        if(i == quick_idx) continue;
        if(d->quick_item[i] == inv_idx) {
            d->quick_item[i] = d->quick_item[quick_idx];
            break;
        }
    }
    d->quick_item[quick_idx] = inv_idx;
}

/*  Update code */
static bork_entity_t get_looked_item(struct bork_play_data* d)
{
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    int i;
    bork_entity_t looked_id = -1;
    bork_entity_t ent_id;
    struct bork_entity* ent = NULL;
    float closest_angle = 0.3f, closest_dist = 2.5f;
    ARR_FOREACH(d->plr_item_query, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent || ent->flags & BORK_ENTFLAG_NOT_INTERACTIVE) continue;
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, ent->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        looked_id = ent_id;
    }
    return looked_id;
}

static bork_entity_t get_looked_enemy(struct bork_play_data* d)
{
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    int i;
    bork_entity_t looked_id = -1;
    bork_entity_t ent_id;
    struct bork_entity* ent = NULL;
    float closest_angle = 0.75f, closest_dist = 25.0f;
    ARR_FOREACH(d->plr_enemy_query, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent) continue;
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, ent->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir)) * dist;
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        looked_id = ent_id;
    }
    return looked_id;
}

static struct bork_map_object* get_looked_map_object(struct bork_play_data* d)
{
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    int i;
    struct bork_map_object* looked_obj = NULL;
    struct bork_map_object* obj;
    float closest_angle = 0.3f, closest_dist = 2.5f;
    ARR_FOREACH_PTR(d->map.doorpads, obj, i) {
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, obj->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        closest_angle = angle;
        looked_obj = obj;
    }
    return looked_obj;
}


static void tick_held_item(struct bork_play_data* d);
static void tick_datapad(struct bork_play_data* d);
static void tick_control_play(struct bork_play_data* d);
static void tick_doorpad(struct bork_play_data* d);
static void tick_control_inv_menu(struct bork_play_data* d);
static void tick_enemies(struct bork_play_data* d);
static void tick_items(struct bork_play_data* d);
static void tick_bullets(struct bork_play_data* d);
static void tick_particles(struct bork_play_data* d);
static void tick_fires(struct bork_play_data* d);

static void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    ++d->ticks;
    /*  Handle input    */
    if(d->menu.state == BORK_MENU_INVENTORY) tick_control_inv_menu(d);
    else if(d->menu.state == BORK_MENU_DOORPAD) tick_doorpad(d);
    else if(d->menu.state == BORK_MENU_PLAYERDEAD) {
        if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)) {
            struct bork_game_core* core = d->core;
            bork_play_deinit(d);
            bork_menu_start(state, core);
            pg_flush_input();
            return;
        }
    } else if(d->menu.state == BORK_MENU_CLOSED) {
        if(d->plr.pain_ticks > 0) --d->plr.pain_ticks;
        vec3 surr_start, surr_end;
        vec3_sub(surr_start, d->plr.pos, (vec3){ 3, 3, 3 });
        vec3_add(surr_end, d->plr.pos, (vec3){ 3, 3, 3 });
        ARR_TRUNCATE(d->plr_enemy_query, 0);
        ARR_TRUNCATE(d->plr_item_query, 0);
        bork_map_query_enemies(&d->map, &d->plr_enemy_query, surr_start, surr_end);
        bork_map_query_items(&d->map, &d->plr_item_query, surr_start, surr_end);
        int x, y, z;
        x = floor(d->plr.pos[0] / 2);
        y = floor(d->plr.pos[1] / 2);
        z = floor(d->plr.pos[2] / 2);
        printf("Player: %d\n", d->map.plr_dist[x][y][z]);
        tick_control_play(d);
        /*  Player update   */
        tick_held_item(d);
        vec2_set(d->plr.dir, d->plr.dir[0] + d->mouse_motion[0],
                             d->plr.dir[1] + d->mouse_motion[1]);
        vec2_set(d->mouse_motion, 0, 0);
        d->plr.dir[0] = fmodf(d->plr.dir[0], M_PI * 2.0f);
        bork_entity_update(&d->plr, &d->map);
        if(d->plr.HP <= 0) d->menu.state = BORK_MENU_PLAYERDEAD;
        /*  Everything else update  */
        bork_map_update(&d->map, &d->plr);
        tick_bullets(d);
        tick_enemies(d);
        tick_items(d);
        tick_fires(d);
        tick_particles(d);
        tick_datapad(d);
        d->looked_item = get_looked_item(d);
        d->looked_enemy = get_looked_enemy(d);
    }
    pg_flush_input();
}

static void tick_control_play(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(pg_check_input(SDL_SCANCODE_TAB, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_INVENTORY;
        d->menu.inv.selection_idx = 0;
        d->menu.inv.scroll_idx = 0;
        SDL_ShowCursor(SDL_ENABLE);
        pg_mouse_mode(0);
    }
    if(pg_check_input(SDL_SCANCODE_M, PG_CONTROL_HIT)) {
        bork_map_build_plr_dist(&d->map);
    }
    if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_X, PG_CONTROL_HIT)) {
        struct bork_entity* item = bork_entity_get(d->looked_item);
        vec3 eye_pos = { d->plr.pos[0], d->plr.pos[1], d->plr.pos[2] };
        if(d->plr.flags & BORK_ENTFLAG_CROUCH) eye_pos[2] += 0.2;
        else eye_pos[2] += 0.8;
        if(item && bork_map_check_vis(&d->map, eye_pos, item->pos)) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
            if(item->flags & BORK_ENTFLAG_IS_AMMO) {
                int ammo_type = item->type - BORK_ITEM_BULLETS;
                d->ammo[ammo_type] += prof->ammo_capacity;
                item->flags |= BORK_ENTFLAG_DEAD;
            } else if(item->type == BORK_ITEM_DATAPAD) {
                d->hud_datapad_id = item->counter[0];
                d->hud_datapad_ticks = 5 * 60;
                d->hud_datapad_line = 0;
                item->flags |= BORK_ENTFLAG_DEAD;
            } else {
                item->flags |= BORK_ENTFLAG_IN_INVENTORY;
                add_inventory_item(d, d->looked_item);
            }
        } else if(!item) {
            struct bork_map_object* obj = get_looked_map_object(d);
            if(obj) {
                struct bork_map_object* door = &d->map.doors.data[obj->doorpad.door_idx];
                if(door->door.locked) {
                    d->menu.doorpad.unlocked_ticks = 0;
                    d->menu.doorpad.selection[0] = -1;
                    d->menu.doorpad.selection[1] = 0;
                    d->menu.doorpad.num_chars = 0;
                    d->menu.doorpad.door_idx = obj->doorpad.door_idx;
                    d->menu.state = BORK_MENU_DOORPAD;
                    SDL_ShowCursor(SDL_ENABLE);
                    pg_mouse_mode(0);
                } else {
                    door->door.open = 1 - door->door.open;
                }
            }
        }
    }
    if(d->held_item >= 0 && pg_check_input(SDL_SCANCODE_R, PG_CONTROL_HIT)) {
        struct bork_entity* ent = bork_entity_get(d->inventory.data[d->held_item]);
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
        if(ent->flags & BORK_ENTFLAG_IS_GUN) {
            int ammo_type = prof->use_ammo[ent->ammo_type] - BORK_ITEM_BULLETS;
            int ammo_missing = prof->ammo_capacity - ent->ammo;
            int ammo_held = MIN(d->ammo[ammo_type], ammo_missing);
            if(ammo_held > 0) {
                d->reload_ticks = prof->reload_time;
                d->reload_length = prof->reload_time;
            }
        }
    }
    if((pg_check_input(kmap[BORK_CTRL_JUMP], PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT))
    && d->plr.flags & BORK_ENTFLAG_GROUND) {
        d->plr.vel[2] = 0.15;
        d->plr.flags &= ~BORK_ENTFLAG_GROUND;
    }
    if(pg_check_input(SDL_SCANCODE_LCTRL, PG_CONTROL_HELD)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSTICK, PG_CONTROL_HELD)) {
        d->plr.flags |= BORK_ENTFLAG_CROUCH;
    } else if(d->plr.flags & BORK_ENTFLAG_CROUCH) {
        if(!bork_map_check_ellipsoid(&d->map,
            (vec3){ d->plr.pos[0], d->plr.pos[1], d->plr.pos[2] + 0.5 },
            BORK_ENT_PROFILES[BORK_ENTITY_PLAYER].size))
        {
            d->plr.flags &= ~BORK_ENTFLAG_CROUCH;
            d->plr.pos[2] += 0.5;
        }
    }
    int kbd_input = 0;
    float move_speed = d->player_speed * (d->plr.flags & BORK_ENTFLAG_GROUND ? 1 : 0.1);
    d->plr.flags &= ~BORK_ENTFLAG_SLIDE;
    if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        d->plr.vel[0] -= move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * cos(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        d->plr.vel[0] += move_speed * sin(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * cos(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        d->plr.vel[0] += move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] += move_speed * sin(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        d->plr.vel[0] -= move_speed * cos(d->plr.dir[0]);
        d->plr.vel[1] -= move_speed * sin(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(!kbd_input) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        float c = cos(-d->plr.dir[0]), s = sin(d->plr.dir[0]);
        vec2 stick_rot = {
            stick[0] * s - stick[1] * c,
            stick[0] * c + stick[1] * s };
        d->plr.vel[0] += move_speed * stick_rot[0];
        d->plr.vel[1] -= move_speed * stick_rot[1];
    }
    if(pg_check_input(SDL_SCANCODE_F, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_Y, PG_CONTROL_HIT)) {
        d->flashlight_on = 1 - d->flashlight_on;
    }
    if(d->held_item != -1) {
        bork_entity_t held_id = d->inventory.data[d->held_item];
        struct bork_entity* held_ent = bork_entity_get(held_id);
        if(held_ent) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[held_ent->type];
            if(!d->reload_ticks && prof->use_ctrl && (pg_check_input(kmap[BORK_CTRL_FIRE], prof->use_ctrl)
            || pg_check_gamepad(PG_RIGHT_TRIGGER, prof->use_ctrl))) {
                prof->use_func(held_ent, d);
            }
        }
    }
    if(pg_check_input(kmap[BORK_CTRL_BIND1], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_UP, PG_CONTROL_HIT)) {
        switch_item(d, d->quick_item[0]);
    } else if(pg_check_input(kmap[BORK_CTRL_BIND2], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_LEFT, PG_CONTROL_HIT)) {
        switch_item(d, d->quick_item[1]);
    } else if(pg_check_input(kmap[BORK_CTRL_BIND3], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, PG_CONTROL_HIT)) {
        switch_item(d, d->quick_item[2]);
    } else if(pg_check_input(kmap[BORK_CTRL_BIND4], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_DOWN, PG_CONTROL_HIT)) {
        switch_item(d, d->quick_item[3]);
    }
}

static void tick_held_item(struct bork_play_data* d)
{
    if(d->held_item < 0) return;
    struct bork_entity* item = bork_entity_get(d->inventory.data[d->held_item]);
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
    if(d->reload_ticks) {
        --d->reload_ticks;
        if(d->reload_ticks == d->reload_length / 2) {
            int ammo_type = prof->use_ammo[item->ammo_type] - BORK_ITEM_BULLETS;
            int ammo_missing = prof->ammo_capacity - item->ammo;
            int ammo_held = MIN(d->ammo[ammo_type], ammo_missing);
            if(ammo_held > 0) {
                d->ammo[ammo_type] -= ammo_held;
                item->ammo += ammo_held;
            }
        }
        return;
    } else if(d->hud_anim_active) {
        d->hud_anim_progress += d->hud_anim_speed;
        int anim_idx = floor(d->hud_anim_progress * 4.0f);
        if(d->hud_callback_frame != -1 && anim_idx == d->hud_callback_frame) {
            d->hud_anim_callback(item, d);
            d->hud_callback_frame = -1;
        }
        if(anim_idx >= d->hud_anim_active) {
            if(d->hud_anim_destroy_when_finished) {
                remove_inventory_item(d, d->held_item);
                item->flags |= BORK_ENTFLAG_DEAD;
                d->held_item = -1;
            }
            bork_play_reset_hud_anim(d);
        }
    }
}

static void tick_datapad(struct bork_play_data* d)
{
    if(d->hud_datapad_id < 0) return;
    const struct bork_datapad* dp = &BORK_DATAPADS[d->hud_datapad_id];
    --d->hud_datapad_ticks;
    if(d->hud_datapad_ticks == 0) {
        d->hud_datapad_line += 2;
        if(d->hud_datapad_line >= dp->lines) d->hud_datapad_id = -1;
        else d->hud_datapad_ticks = 5 * 60;
    }
}

static void tick_enemy(struct bork_play_data* d, struct bork_entity* ent)
{
    tin_canine_tick(d, ent);
}

static void tick_enemies(struct bork_play_data* d)
{
    int x, y, z, i;
    for(x = 0; x < 4; ++x)
    for(y = 0; y < 4; ++y)
    for(z = 0; z < 4; ++z) {
        bork_entity_t ent_id;
        struct bork_entity* ent;
        ARR_FOREACH_REV(d->map.enemies[x][y][z], ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent) {
                ARR_SWAPSPLICE(d->map.enemies[x][y][z], i, 1);
                continue;
            }
            if(ent->last_tick == d->ticks) continue;
            else if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                ent->last_tick = d->ticks;
                tick_enemy(d, ent);
            } else {
                ARR_SWAPSPLICE(d->map.enemies[x][y][z], i, 1);
                bork_map_add_enemy(&d->map, ent_id);
            }
        }
    }
}

static void tick_item(struct bork_play_data* d, struct bork_entity* ent)
{
    if(ent->flags & BORK_ENTFLAG_SMOKING) {
        if(ent->counter[3] % 60 == 0) {
            create_smoke(d, 
                (vec3){ ent->pos[0] + (RANDF - 0.5) * 0.5,
                        ent->pos[1] + (RANDF - 0.5) * 0.5,
                        ent->pos[2] + (RANDF - 0.5) * 0.5 },
                (vec3){}, 120);
        }
        --ent->counter[3];
        if(ent->counter[3] <= 0) ent->flags &= ~BORK_ENTFLAG_SMOKING;
    }
    bork_entity_update(ent, &d->map);
}

static void tick_items(struct bork_play_data* d)
{
    int x, y, z, i;
    for(x = 0; x < 4; ++x)
    for(y = 0; y < 4; ++y)
    for(z = 0; z < 4; ++z) {
        bork_entity_t ent_id;
        struct bork_entity* ent;
        ARR_FOREACH_REV(d->map.items[x][y][z], ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent || (ent->flags & (BORK_ENTFLAG_IN_INVENTORY | BORK_ENTFLAG_DEAD))) {
                ARR_SWAPSPLICE(d->map.items[x][y][z], i, 1);
                continue;
            }
            if(ent->last_tick == d->ticks) continue;
            else if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                ent->last_tick = d->ticks;
                tick_item(d, ent);
            } else {
                ARR_SWAPSPLICE(d->map.items[x][y][z], i, 1);
                bork_map_add_item(&d->map, ent_id);
            }
        }
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

static void tick_particles(struct bork_play_data* d)
{
    struct bork_particle* part;
    int i;
    ARR_FOREACH_PTR_REV(d->particles, part, i) {
        if(part->ticks_left <= 0) {
            ARR_SWAPSPLICE(d->particles, i, 1);
            continue;
        }
        --part->ticks_left;
        if(part->frame_ticks && part->ticks_left % part->frame_ticks == 0) {
            if(part->flags & BORK_PARTICLE_LOOP_ANIM) {
                if(part->current_frame == part->end_frame)
                    part->current_frame = part->start_frame;
                else ++part->current_frame;
            } else if(part->current_frame < part->end_frame) {
                ++part->current_frame;
            }
        }
        vec3_add(part->pos, part->pos, part->vel);
        if(part->flags & BORK_PARTICLE_GRAVITY) part->vel[2] -= 0.001;
        else if(part->flags & BORK_PARTICLE_BOUYANT) part->vel[2] += 0.00025;
    }
}

static void tick_fires(struct bork_play_data* d)
{
    struct bork_fire* fire;
    int i;
    ARR_FOREACH_REV_PTR(d->map.fires, fire, i) {
        if(fire->lifetime % 41 == 0) create_smoke(d, fire->pos, (vec3){}, 180);
        else if(fire->lifetime % 31 == 0) {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT | BORK_PARTICLE_DECELERATE,
                .pos = { fire->pos[0], fire->pos[1], fire->pos[2] },
                .vel = { 0, 0, -0.005 },
                .ticks_left = 120,
                .frame_ticks = 0,
                .current_frame = 24 + rand() % 4,
            };
            ARR_PUSH(d->particles, new_part);
        }
        --fire->lifetime;
        if(fire->lifetime == 0) ARR_SWAPSPLICE(d->map.fires, i, 1);
    }
}

/*  Drawing */
static void draw_hud_overlay(struct bork_play_data* d);
static void draw_datapad(struct bork_play_data* d);
static void draw_gameover(struct bork_play_data* d, float t);
static void draw_weapon(struct bork_play_data* d, float hud_anim_lerp,
                        vec3 pos_lerp, vec2 dir_lerp);
static void draw_enemies(struct bork_play_data* d, float lerp);
static void draw_items(struct bork_play_data* d, float lerp);
static void draw_bullets(struct bork_play_data* d, float lerp);
static void draw_map_lights(struct bork_play_data* d);
static void draw_lights(struct bork_play_data* d);
static void draw_particles(struct bork_play_data* d);
static void draw_fires(struct bork_play_data* d);
static void draw_menu_inv(struct bork_play_data* d, float t);
static void draw_doorpad(struct bork_play_data* d, float t);
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
    vec3 draw_coll_size;
    vec3_dup(draw_coll_size, BORK_ENT_PROFILES[BORK_ENTITY_PLAYER].size);
    if(d->plr.flags & BORK_ENTFLAG_CROUCH) draw_coll_size[2] *= 0.5;
    struct bork_collision draw_collision = {};
    bork_map_collide(&d->map, &draw_collision, draw_pos, draw_coll_size);
    vec3_add(draw_pos, draw_pos, draw_collision.push);
    vec3_add(draw_pos, draw_pos, (vec3){ 0, 0, draw_coll_size[2] * 0.9 });
    vec2_add(draw_dir, d->plr.dir, d->mouse_motion);
    pg_viewer_set(&d->core->view, draw_pos, draw_dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->core->gbuf);
    pg_shader_begin(&d->core->shader_3d, &d->core->view);
    draw_weapon(d, d->hud_anim_progress + (d->hud_anim_speed * state->tick_over),
                draw_pos, draw_dir);
    pg_shader_begin(&d->core->shader_sprite, &d->core->view);
    draw_enemies(d, t);
    draw_items(d, t);
    draw_bullets(d, t);
    draw_fires(d);
    draw_particles(d);
    pg_shader_3d_texture(&d->core->shader_3d, &d->core->env_atlas);
    bork_map_draw(&d->map, d->core);
    draw_map_lights(d);
    if(d->flashlight_on) {
        struct pg_light flashlight;
        vec3 look_dir;
        spherical_to_cartesian(look_dir, (vec2){ draw_dir[0] - M_PI,
                                                 draw_dir[1] - (M_PI * 0.5) });
        vec3 flash_pos;
        vec3_dup(flash_pos, draw_pos);
        if(d->plr.flags & BORK_ENTFLAG_CROUCH) flash_pos[2] -= 0.2;
        else flash_pos[2] -= 0.4;
        pg_light_spotlight(&flashlight, flash_pos, 12, (vec3){ 1, 1, 0.75 },
                           look_dir, 0.35);
        ARR_PUSH(d->spotlights, flashlight);
    }
    draw_lights(d);
    if(d->menu.state == BORK_MENU_CLOSED) {
        pg_screen_dst();
        pg_gbuffer_finish(&d->core->gbuf, (vec3){ 0.05, 0.05, 0.05 });
        draw_hud_overlay(d);
        draw_datapad(d);
    } else {
        /*  Finish to the post-process buffer so we can do the blur effect  */
        pg_ppbuffer_dst(&d->core->ppbuf);
        pg_gbuffer_finish(&d->core->gbuf, (vec3){ 0.05, 0.05, 0.05 });
        pg_postproc_blur_scale(&d->core->post_blur, (vec2){ 3, 3 });
        pg_postproc_blur_dir(&d->core->post_blur, 1);
        pg_ppbuffer_swapdst(&d->core->ppbuf);
        pg_postproc_apply(&d->core->post_blur, &d->core->ppbuf);
        pg_postproc_blur_dir(&d->core->post_blur, 0);
        pg_ppbuffer_swap(&d->core->ppbuf);
        pg_screen_dst();
        pg_postproc_apply(&d->core->post_blur, &d->core->ppbuf);
        bork_draw_backdrop(d->core, (vec4){ 0.7, 0.7, 0.7, 0.5 }, t);
        bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 0.75 });
        pg_shader_begin(&d->core->shader_2d, NULL);
        switch(d->menu.state) {
        default: break;
        case BORK_MENU_INVENTORY:
            draw_menu_inv(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_DOORPAD:
            draw_doorpad(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_PLAYERDEAD:
            draw_gameover(d, (float)state->ticks / (float)state->tick_rate);
            break;
        }
    }
    pg_shader_begin(&d->core->shader_text, NULL);
    bork_draw_fps(d->core);
}

static void draw_lights(struct bork_play_data* d)
{
    pg_gbuffer_begin_pointlight(&d->core->gbuf, &d->core->view);
    int i;
    int nearlights = 0;
    struct pg_light* light;
    ARR_FOREACH_PTR(d->lights_buf, light, i) {
        vec3 light_to_plr;
        vec3_sub(light_to_plr, light->pos, d->plr.pos);
        vec3_abs(light_to_plr, light_to_plr);
        if(vec3_vmax(light_to_plr) < light->size * 1.25) {
            d->lights_buf.data[nearlights++] = *light;
            continue;
        }
        pg_gbuffer_draw_pointlight(&d->core->gbuf, light);
    }
    pg_gbuffer_mode(&d->core->gbuf, 1);
    for(i = 0; i < nearlights; ++i) {
        pg_gbuffer_draw_pointlight(&d->core->gbuf, &d->lights_buf.data[i]);
    }
    nearlights = 0;
    pg_gbuffer_begin_spotlight(&d->core->gbuf, &d->core->view);
    ARR_FOREACH_PTR(d->spotlights, light, i) {
        vec3 light_to_plr;
        vec3_sub(light_to_plr, light->pos, d->plr.pos);
        vec3_abs(light_to_plr, light_to_plr);
        if(vec3_vmax(light_to_plr) < light->size * 1.5) {
            d->spotlights.data[nearlights++] = *light;
            continue;
        }
        pg_gbuffer_draw_spotlight(&d->core->gbuf, light);
    }
    pg_gbuffer_mode(&d->core->gbuf, 1);
    for(i = 0; i < nearlights; ++i) {
        pg_gbuffer_draw_spotlight(&d->core->gbuf, &d->spotlights.data[i]);
    }
    ARR_TRUNCATE(d->lights_buf, 0);
    ARR_TRUNCATE(d->spotlights, 0);
}

static void draw_hud_overlay(struct bork_play_data* d)
{
    float ar = d->core->aspect_ratio;
    pg_shader_begin(&d->core->shader_text, NULL);
    pg_shader_text_resolution(&d->core->shader_text, (vec2){ ar, 1 });
    pg_shader_text_transform(&d->core->shader_text, (vec2){ 1, 1 }, (vec2){});
    struct bork_entity* looked_ent;
    if(d->looked_item != -1 && (looked_ent = bork_entity_get(d->looked_item))) {
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[looked_ent->type];
        struct pg_shader_text text = { .use_blocks = 1 };
        snprintf(text.block[0], 64, "%s", prof->name);
        vec4_set(text.block_style[0],
            (ar * 0.5) - strnlen(text.block[0], 64) * 0.04 * 1.25 * 0.5,
            0.3, 0.04, 1.25);
        vec4_set(text.block_color[0], 1, 1, 1, 0.75);
        pg_shader_text_write(&d->core->shader_text, &text);
    }
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
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_tex_frame(&d->core->shader_2d, 56);
    float hp_frac = (float)d->plr.HP / 100.0f;
    pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ hp_frac * 4, 1 }, (vec2){ 0, 0 });
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ ar / 2 - 0.2, 0.86 }, (vec2){ 0.4 * hp_frac, 0.1 }, 0);
    pg_model_draw(&d->core->quad_2d, NULL);
    if(d->plr.pain_ticks > 0) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){}, (vec2){ ar, 1 }, 0);
        pg_shader_2d_texture(&d->core->shader_2d, &d->core->radial_vignette);
        float hurt_alpha = (float)d->plr.pain_ticks / 120 * 0.5;
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 0.5, 0, 0, hurt_alpha });
        pg_model_draw(&d->core->quad_2d, NULL);
    }
    if(d->held_item != -1) {
    }
}

static void draw_datapad(struct bork_play_data* d)
{
    if(d->hud_datapad_id < 0) return;
    const struct bork_datapad* dp = &BORK_DATAPADS[d->hud_datapad_id];
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_2d_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_2d_set_light(shader, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    pg_shader_begin(shader, NULL);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    pg_shader_2d_tex_frame(shader, 4);
    pg_shader_2d_transform(shader, (vec2){ 0.3, 0.84 }, (vec2){ 0.1, 0.1 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_tex_frame(shader, 52);
    pg_shader_2d_add_tex_tx(shader, (vec2){ 4, 1.5 }, (vec2){});
    pg_shader_2d_transform(shader, (vec2){ 0.3, 0.75 }, (vec2){ 0.25, 0.1 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = { .use_blocks = 2 };
    int len = snprintf(text.block[0], 64, "%s", dp->title);
    vec4_set(text.block_style[0], 0.3 - (len * 0.0175 * 1.125 * 0.5), 0.95, 0.0175, 1.125);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    len = snprintf(text.block[1], 64, "%s", dp->text[d->hud_datapad_line]);
    vec4_set(text.block_style[1], 0.3 - (len * 0.0175 * 1.125 * 0.5), 0.6, 0.0175, 1.125);
    vec4_set(text.block_color[1], 1, 1, 1, 1);
    if(d->hud_datapad_line < dp->lines - 1) {
        text.use_blocks = 3;
        len = snprintf(text.block[2], 64, "%s", dp->text[d->hud_datapad_line + 1]);
        vec4_set(text.block_style[2], 0.3 - (len * 0.0175 * 1.125 * 0.5), 0.63, 0.0175, 1.125);
        vec4_set(text.block_color[2], 1, 1, 1, 1);
    }
    pg_shader_text_write(shader, &text);
}

static void draw_weapon(struct bork_play_data* d, float hud_anim_lerp,
                        vec3 pos_lerp, vec2 dir_lerp)
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
    vec3 hud_pos = {};
    float hud_angle = 0;
    mat4 offset;
    mat4_translate(offset, -0.6, 0.3, -0.2);
    if(d->reload_ticks) {
        int down_time = d->reload_length - d->reload_ticks;
        float t = 1;
        if(down_time < 31) {
            t = (float)down_time / 30;
        } else if(d->reload_ticks < 31) {
            t = (float)d->reload_ticks / 30;
        }
        mat4_translate_in_place(offset, 0, 0, -t);
    } else {
        /*  Calculate the animated position and add it to the offset    */
        int anim_idx = floor(hud_anim_lerp * 4.0f);
        float anim_between = fmodf(hud_anim_lerp, 0.25) * 4.0f;
        vec3_lerp(hud_pos, d->hud_anim[anim_idx], d->hud_anim[(anim_idx + 1) % 4], anim_between);
        hud_angle = lerp(d->hud_angle[anim_idx], d->hud_angle[(anim_idx + 1) % 4], anim_between);
        /*  Apply the animated transform    */
        mat4_translate_in_place(offset, hud_pos[0], hud_pos[1], hud_pos[2]);
    }
    mat4_mul(tx, tx, offset);
    mat4_scale_aniso(tx, tx, prof->sprite_tx[0], prof->sprite_tx[1], prof->sprite_tx[1]);
    mat4_rotate_Y(tx, tx, hud_angle);
    pg_model_begin(model, shader);
    pg_model_draw(model, tx);
}

static void draw_enemies(struct bork_play_data* d, float lerp)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->enemy_model;
    int current_frame = 0;
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_texture(shader, &d->core->env_atlas);
    pg_shader_sprite_tex_frame(shader, 0);
    pg_shader_sprite_add_tex_tx(shader, (vec2){ 1, 1 }, (vec2){});
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){});
    pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(model, shader);
    int x, y, z, i;
    for(x = 0; x < 4; ++x)
    for(y = 0; y < 4; ++y)
    for(z = 0; z < 4; ++z) {
        bork_entity_t ent_id;
        struct bork_entity* ent;
        const struct bork_entity_profile* prof;
        ARR_FOREACH(d->map.enemies[x][y][z], ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent) continue;
            prof = &BORK_ENT_PROFILES[ent->type];
            /*  Figure out which directional frame to draw  */
            vec3 pos_lerp;
            vec3_scale(pos_lerp, ent->vel, lerp);
            vec3_add(pos_lerp, pos_lerp, ent->pos);
            if((ent->flags & BORK_ENTFLAG_DYING) && ent->dead_ticks <= 60) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light, pos_lerp,
                    ((float)ent->dead_ticks / 60.0f) * 5.0f, (vec3){ 1.5, 0.25, 0.25 });
                ARR_PUSH(d->lights_buf, new_light);
                continue;
            } else if(ent->flags & BORK_ENTFLAG_ON_FIRE) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light, pos_lerp, 2.5, (vec3){ 2.0, 1.5, 0 });
                ARR_PUSH(d->lights_buf, new_light);
            }
            vec2 ent_to_plr;
            vec2_sub(ent_to_plr, pos_lerp, d->plr.pos);
            float dir_adjust = 1.0f / (float)prof->dir_frames + 0.5f;
            float angle = atan2(ent_to_plr[0], ent_to_plr[1]) - dir_adjust + M_PI + ent->dir[0];
            angle = fmodf(angle, M_PI * 2);
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
}

static void draw_items(struct bork_play_data* d, float lerp)
{
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
    int x, y, z, i;
    for(x = 0; x < 4; ++x)
    for(y = 0; y < 4; ++y)
    for(z = 0; z < 4; ++z) {
        bork_entity_t ent_id;
        struct bork_entity* ent;
        const struct bork_entity_profile* prof;
        ARR_FOREACH(d->map.items[x][y][z], ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent) continue;
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
            mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2] + 0.05);
            mat4_scale(transform, transform, 0.5);
            if(ent_id == d->looked_item) {
                pg_shader_sprite_color_mod(shader, (vec4){ 1.5f, 1.8f, 1.5f, 1.0f });
                pg_model_draw(model, transform);
                pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
            } else pg_model_draw(model, transform);
        }
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
            struct pg_light light;
            pg_light_pointlight(&light, blt->pos,
                ((float)blt->dead_ticks / 10.0f) * 1.0f, blt->light_color);
            ARR_PUSH(d->lights_buf, light);
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
    struct pg_light* light;
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

static void draw_fires(struct bork_play_data* d)
{
    struct pg_light light;
    pg_light_pointlight(&light, (vec3){}, 2.5, (vec3){ 2.0, 1.5, 0 });
    struct bork_fire* fire;
    int i;
    ARR_FOREACH_PTR(d->map.fires, fire, i) {
        vec3_dup(light.pos, fire->pos);
        ARR_PUSH(d->lights_buf, light);
    }
}

static void draw_particles(struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->enemy_model;
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    pg_shader_sprite_texture(shader, &d->core->particle_tex);
    pg_shader_sprite_tex_frame(shader, 0);
    pg_shader_sprite_mode(shader, PG_SPRITE_SPHERICAL);
    pg_shader_sprite_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    pg_model_begin(model, shader);
    mat4 part_transform;
    struct bork_particle* part;
    int i;
    ARR_FOREACH_PTR(d->particles, part, i) {
        pg_shader_sprite_tex_frame(shader, part->current_frame);
        mat4_translate(part_transform, part->pos[0], part->pos[1], part->pos[2]);
        pg_model_draw(model, part_transform);
    }
}

static void draw_quickfetch_items(struct bork_play_data* d,
                                  vec4 color_mod, vec4 selected_mod)
{
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    float w = d->core->aspect_ratio;
    pg_shader_2d_resolution(shader, (vec2){ w, 1 });
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_tex_frame(shader, 0);
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    vec2 light_pos = { sin((float)d->ticks / 60.0f / M_PI), cos((float)d->ticks / 60.0f / M_PI) };
    vec2_scale(light_pos, light_pos, 0.3);
    vec2_add(light_pos, light_pos, (vec2){ w - 0.2, 0.8 });
    pg_shader_2d_set_light(shader, light_pos, (vec3){ 1.5, 1.5, 1.4 },
                           (vec3){ 0.5, 0.5, 0.5 });
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
        pg_shader_2d_transform(shader,
            (vec2){ draw_pos[i][0], draw_pos[i][1] - 0.05 * prof->inv_height },
            (vec2){ 0.05 * prof->sprite_tx[0], 0.05 * prof->sprite_tx[1] },
            prof->inv_angle);
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
        .use_blocks = draw_label ? 10 : 8,
        .block = {
            "1", "2", "3", "4", "", "", "", "",
            "QUICK", "FETCH",
        },
        .block_style = {
            { w - 0.2, 0.8, 0.03, 1 }, { w - 0.25, 0.85, 0.03, 1 },
            { w - 0.15, 0.85, 0.03, 1 }, { w - 0.2, 0.9, 0.03, 1 },
            { w - 0.17, 0.805, 0.02, 1 }, { w - 0.22, 0.855, 0.02, 1 },
            { w - 0.12, 0.855, 0.02, 1 }, { w - 0.17, 0.905, 0.02, 1 },
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
        if(d->held_item >= 0 && d->quick_item[i] == d->held_item) {
            vec4_mul(text.block_color[i], text.block_color[i], selected_mod);
            vec4_mul(text.block_color[i + 4], text.block_color[i], selected_mod);
        } else {
            vec4_mul(text.block_color[i], text.block_color[i], color_mod);
            vec4_mul(text.block_color[i + 4], text.block_color[i], selected_mod);
        }
        if(d->quick_item[i] < 0) continue;
        struct bork_entity* ent = bork_entity_get(d->inventory.data[d->quick_item[i]]);
        if(!ent || !(ent->flags & BORK_ENTFLAG_STACKS)) continue;
        snprintf(text.block[i + 4], 5, "x%d", ent->item_quantity);
    }
    pg_shader_text_write(&d->core->shader_text, &text);
};

static void draw_inventory_text(struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_text;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    float ar = d->core->aspect_ratio;
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int inv_len = MIN(10, d->inventory.len);
    int inv_start = d->menu.inv.scroll_idx;
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
        vec4_set(text.block_style[ti], 0.1, 0.2 + 0.06 * i, 0.03, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.5);
        if(i + inv_start == d->menu.inv.selection_idx) {
            text.block_style[ti][0] += 0.05;
            text.block_color[ti][3] = 0.75;
        }
    }
    if(d->inventory.len > 0) {
        struct bork_entity* item = bork_entity_get(d->inventory.data[d->menu.inv.selection_idx]);
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
        if(item->flags & BORK_ENTFLAG_IS_GUN) {
            const struct bork_entity_profile* ammo_prof =
                &BORK_ENT_PROFILES[prof->use_ammo[d->menu.inv.ammo_select]];
            int len = snprintf(text.block[ti], 64, "%s", ammo_prof->name);
            vec4_set(text.block_style[ti], ar * 0.75 - 0.25, 0.5, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
            len = snprintf(text.block[++ti], 64, "DAMAGE:     %d",
                prof->damage + ammo_prof->damage);
            vec4_set(text.block_style[ti], ar * 0.75 - 0.25, 0.54, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
            len = snprintf(text.block[++ti], 64,   "ARMOR PEN.: %d",
                prof->armor_pierce + ammo_prof->armor_pierce);
            vec4_set(text.block_style[ti], ar * 0.75 - 0.25, 0.58, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
            len = snprintf(text.block[++ti], 64,   "FIRE RATE:  %d", prof->speed);
            vec4_set(text.block_style[ti], ar * 0.75 - 0.25, 0.62, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
            int j;
            for(j = 0; j < prof->ammo_types; ++j) {
                int held_ammo = d->ammo[prof->use_ammo[j] - BORK_ITEM_BULLETS];
                len = snprintf(text.block[++ti], 64, "%d", held_ammo);
                float x_off = len * 0.025 * 1.2 * 0.5;
                vec4_set(text.block_style[ti], ar * 0.75 - 0.2 + 0.15 * j - x_off, 0.44, 0.025, 1.2);
                vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
            }
        }
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(&d->core->shader_text, &text);
}

static void draw_menu_inv(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    vec2 light_pos = { ar * 0.75 + (sin((float)d->ticks / 120.0f / M_PI) * 0.5f), 0 };
    pg_shader_2d_set_light(shader, light_pos, (vec3){ 1.5, 1.5, 1.25 },
                           (vec3){ 0.5, 0.5, 0.5 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    if(d->inventory.len > 0) {
        struct bork_entity* item = bork_entity_get(d->inventory.data[d->menu.inv.selection_idx]);
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
        pg_shader_2d_tex_frame(shader, prof->front_frame);
        pg_shader_2d_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
        pg_shader_2d_transform(shader,
            (vec2){ ar * 0.75, 0.25 - (0.18 * prof->inv_height) },
            (vec2){ 0.18 * prof->sprite_tx[0], 0.18 * prof->sprite_tx[1] },
            prof->inv_angle);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        if(item->flags & BORK_ENTFLAG_IS_GUN) {
            int i;
            for(i = 0; i < prof->ammo_types; ++i) {
                const struct bork_entity_profile* ammo_prof =
                    &BORK_ENT_PROFILES[prof->use_ammo[i]];
                if(i != d->menu.inv.ammo_select) {
                    pg_shader_2d_transform(shader,
                        (vec2){ ar * 0.75 - 0.2 + 0.15 * i, 0.43 },
                        (vec2){ 0.05, 0.05 }, 0);
                    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.5 });
                } else {
                    pg_shader_2d_transform(shader,
                        (vec2){ ar * 0.75 - 0.2 + 0.15 * i, 0.43 },
                        (vec2){ 0.075, 0.075 }, 0);
                    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
                }
                pg_shader_2d_tex_frame(shader, ammo_prof->front_frame);
                pg_model_draw(&d->core->quad_2d_ctr, NULL);
            }
        }
    }
    draw_inventory_text(d);
    draw_quickfetch_text(d, 1, (vec4){ 1, 1, 1, 0.75 }, (vec4){ 1, 1, 1, 0.9 });
    draw_quickfetch_items(d, (vec4){ 1, 1, 1, 0.75 }, (vec4){ 1, 1, 1, 0.9 });
}

static void tick_control_inv_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
    || pg_check_input(SDL_SCANCODE_TAB, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_CLOSED;
        pg_mouse_mode(1);
    }
    if(d->inventory.len == 0) return;
    struct bork_entity* item = bork_entity_get(d->inventory.data[d->menu.inv.selection_idx]);
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    int start_select = d->menu.inv.selection_idx;
    if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1) {
        d->menu.inv.selection_idx = MIN(d->menu.inv.selection_idx + 1, d->inventory.len - 1);
        if(d->menu.inv.selection_idx >= d->menu.inv.scroll_idx + 10) ++d->menu.inv.scroll_idx;
    } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
        d->menu.inv.selection_idx = MAX(d->menu.inv.selection_idx - 1, 0);
        if(d->menu.inv.selection_idx < d->menu.inv.scroll_idx) --d->menu.inv.scroll_idx;
    }
    if(d->menu.inv.selection_idx != start_select) {
        item = bork_entity_get(d->inventory.data[d->menu.inv.selection_idx]);
        prof = &BORK_ENT_PROFILES[item->type];
        if(item->flags & BORK_ENTFLAG_IS_GUN) {
            d->menu.inv.ammo_select = item->ammo_type;
        }
    }
    if(item->flags & BORK_ENTFLAG_IS_GUN) {
        int ammo_iter = d->menu.inv.ammo_select;
        if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT) || stick_ctrl_x == -1) {
            ammo_iter = MOD(ammo_iter - 1, prof->ammo_types);
            while(ammo_iter != d->menu.inv.ammo_select
            && d->ammo[prof->use_ammo[ammo_iter] - BORK_ITEM_BULLETS] <= 0) {
                ammo_iter = MOD(ammo_iter - 1, prof->ammo_types);
            }
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT) || stick_ctrl_x == 1) {
            ammo_iter = MOD(ammo_iter + 1, prof->ammo_types);
            while(ammo_iter != d->menu.inv.ammo_select
            && d->ammo[prof->use_ammo[ammo_iter] - BORK_ITEM_BULLETS] <= 0) {
                ammo_iter = MOD(ammo_iter + 1, prof->ammo_types);
            }
        }
        d->menu.inv.ammo_select = ammo_iter;
        int ammo_type = prof->use_ammo[ammo_iter] - BORK_ITEM_BULLETS;
        int used_ammo = prof->use_ammo[item->ammo_type] - BORK_ITEM_BULLETS;
        if(pg_check_input(SDL_SCANCODE_E, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_X, PG_CONTROL_HIT)) {
            if(d->menu.inv.ammo_select == item->ammo_type) {
                d->ammo[ammo_type] += item->ammo;
                item->ammo = 0;
            } else if(d->ammo[ammo_type] > 0) {
                d->ammo[used_ammo] += item->ammo;
                item->ammo = 0;
                item->ammo_type = ammo_iter;
                if(d->menu.inv.selection_idx == d->held_item) {
                    d->reload_ticks = prof->reload_time;
                    d->reload_length = prof->reload_time;
                }
            }
        }
    }
    if(pg_check_input(kmap[BORK_CTRL_BIND1], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_UP, PG_CONTROL_HIT)) {
        set_quick_item(d, 0, d->menu.inv.selection_idx);
    }
    if(pg_check_input(kmap[BORK_CTRL_BIND2], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_LEFT, PG_CONTROL_HIT)) {
        set_quick_item(d, 1, d->menu.inv.selection_idx);
    }
    if(pg_check_input(kmap[BORK_CTRL_BIND3], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, PG_CONTROL_HIT)) {
        set_quick_item(d, 2, d->menu.inv.selection_idx);
    }
    if(pg_check_input(kmap[BORK_CTRL_BIND4], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_DPAD_DOWN, PG_CONTROL_HIT)) {
        set_quick_item(d, 3, d->menu.inv.selection_idx);
    }
}

static void tick_doorpad(struct bork_play_data* d)
{
    if(d->menu.doorpad.unlocked_ticks > 0) {
        --d->menu.doorpad.unlocked_ticks;
        if(d->menu.doorpad.unlocked_ticks == 0) {
            d->menu.state = BORK_MENU_CLOSED;
            pg_mouse_mode(1);
        }
        return;
    }
    if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_CLOSED;
        pg_mouse_mode(1);
    }
    float ar = d->core->aspect_ratio;
    struct bork_map_object* door = &d->map.doors.data[d->menu.doorpad.door_idx];
    uint8_t* chars = d->menu.doorpad.chars;
    const uint8_t* door_chars = door->door.code;
    vec2 button_pos[12];
    int i;
    for(i = 0; i < 12; ++i) {
        vec2_set(button_pos[i], (i % 3) * 0.1325 + ar * 0.5 - 0.06,
                                (i / 3) * 0.11 + 0.39);
    }
    if(pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT)) {
        vec2 mouse_pos;
        pg_mouse_pos(mouse_pos);
        vec2 click = { mouse_pos[0] / d->core->screen_size[1],
                       mouse_pos[1] / d->core->screen_size[1] };
        for(i = 0; i < 12; ++i) {
            vec2 diff;
            vec2_sub(diff, click, button_pos[i]);
            vec2_add(diff, diff, (vec2){ 0.025, 0.02 });
            if(diff[0] < 0 || diff[1] < 0
            || diff[0] > 0.08 || diff[1] > 0.065) continue;
            if(i < 10 && d->menu.doorpad.num_chars < 4) {
                chars[d->menu.doorpad.num_chars++] = MOD(i + 1, 10);
            } else if(i == 10) {
                d->menu.doorpad.num_chars = MAX(0, d->menu.doorpad.num_chars - 1);
            } else if(i == 11 && d->menu.doorpad.num_chars == 4
            && chars[0] == door_chars[0] && chars[1] == door_chars[1]
            && chars[2] == door_chars[2] && chars[3] == door_chars[3]) {
                door->door.locked = 0;
                door->door.open = 1;
                d->menu.doorpad.unlocked_ticks = 60;
            }
        }
    }
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        if(d->menu.doorpad.selection[0] == -1) d->menu.doorpad.selection[0] = 0;
        else {
            vec2 stick;
            pg_gamepad_stick(0, stick);
            if(fabsf(stick[0]) > fabsf(stick[1])) {
                int x = SGN(stick[0]);
                d->menu.doorpad.selection[0] = MOD(d->menu.doorpad.selection[0] + x, 3);
            } else {
                int y = SGN(stick[1]);
                d->menu.doorpad.selection[1] = MOD(d->menu.doorpad.selection[1] + y, 4);
            }
        }
    }
    if(pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
        int b = d->menu.doorpad.selection[0] + d->menu.doorpad.selection[1] * 3;
        if(b < 10 && d->menu.doorpad.num_chars < 4) {
            chars[d->menu.doorpad.num_chars++] = MOD(b + 1, 10);
        } else if(b == 10) {
            d->menu.doorpad.num_chars = MAX(0, d->menu.doorpad.num_chars - 1);
        } else if(b == 11 && d->menu.doorpad.num_chars == 4
        && chars[0] == door_chars[0] && chars[1] == door_chars[1]
        && chars[2] == door_chars[2] && chars[3] == door_chars[3]) {
            door->door.locked = 0;
            door->door.open = 1;
            d->menu.doorpad.unlocked_ticks = 60;
        }
    }
}

static void draw_doorpad(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1.0f });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_texture(shader, &d->core->env_atlas);
    pg_shader_2d_tex_frame(shader, 2);
    vec2 light_pos = { ar * 0.5 + sin((float)d->ticks / 60.0f / M_PI) * 0.5f, 0 };
    pg_shader_2d_set_light(shader, light_pos, (vec3){ 1.5, 1.5, 1.4 },
                           (vec3){ 0.5, 0.5, 0.5 });
    struct bork_map_object* door = &d->map.doors.data[d->menu.doorpad.door_idx];
    if(!door->door.locked) {
        pg_shader_2d_add_tex_tx(shader, (vec2){ 1, 1 }, (vec2){ 0, 144.0f / 512.0f });
    } else {
        pg_shader_2d_add_tex_tx(shader, (vec2){ 1, 1 }, (vec2){ 0, 48.0f / 512.0f });
    }
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    pg_shader_2d_transform(shader, (vec2){ ar * 0.5, 0.5 }, (vec2){ 0.35, 0.35 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    struct pg_shader_text text = { .use_blocks = 13,
        .block = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "X", ">" }
    };
    int b;
    if(d->menu.doorpad.selection[0] != -1) {
        b = d->menu.doorpad.selection[0] + d->menu.doorpad.selection[1] * 3;
    } else b = -1;
    int i;
    for(i = 0; i < 12; ++i) {
        vec4_set(text.block_style[i], (i % 3) * 0.1325 + ar * 0.5 - 0.06,
                                      (i / 3) * 0.11 + 0.39, 0.03, 0);
        if(b == -1 ||  b == i) vec4_set(text.block_color[i], 1, 1, 1, 1);
        else vec4_set(text.block_color[i], 0.8, 0.8, 0.8, 0.6);
    };
    for(i = 0; i < d->menu.doorpad.num_chars; ++i) {
        text.block[12][i] = '0' + d->menu.doorpad.chars[i];
    }
    text.block[12][i] = '\0';
    vec4_set(text.block_style[12], ar * 0.5 - 0.04, 0.265, 0.03, 2.5);
    vec4_set(text.block_color[12], 0, 0, 0, 1);
    pg_shader_text_write(shader, &text);
}

static void draw_gameover(struct bork_play_data* d, float t)
{
    float ctr = d->core->aspect_ratio * 0.5;
    struct pg_shader* shader = &d->core->shader_text;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){d->core->aspect_ratio, 1});
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = { .use_blocks = 3,
        .block = { "YOU DIED", "PRESS ENTER TO RETURN", "TO THE MAIN MENU" },
        .block_style = {
            { ctr - (8 * 0.075 * 1.25 * 0.5), 0.35, 0.075, 1.25 },
            { ctr - (strnlen(text.block[1], 64) * 0.04 * 1.25 * 0.5), 0.55, 0.04, 1.25 },
            { ctr - (strnlen(text.block[2], 64) * 0.04 * 1.25 * 0.5), 0.6, 0.04, 1.25 } },
        .block_color = { { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 } } };
    pg_shader_text_write(shader, &text);
}

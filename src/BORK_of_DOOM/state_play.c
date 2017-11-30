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
#include "upgrades.h"
#include "recycler.h"
#include "game_states.h"
#include "state_play.h"

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

void bork_play_deinit(void* data)
{
    struct bork_play_data* d = data;
    bork_map_deinit(&d->map);
    ARR_DEINIT(d->particles);
    ARR_DEINIT(d->plr_enemy_query);
    ARR_DEINIT(d->plr_item_query);
    ARR_DEINIT(d->plr_entity_query);
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
    d->upgrade_selected = 3;
    d->upgrades[0] = -1;
    d->upgrades[1] = -1;
    d->upgrades[2] = -1;
    d->upgrades[3] = BORK_UPGRADE_SCANNING;
    d->upgrade_level[0] = 0;
    d->upgrade_level[1] = 0;
    d->upgrade_level[2] = 0;
    d->upgrade_level[3] = 0;
    /*  Generate the BONZ station   */
    bork_map_init(&d->map);
    struct bork_editor_map ed_map = {};
    char filename[1024];
    snprintf(filename, 1024, "%stest.bork_map", core->base_path);
    bork_editor_load_map(&ed_map, filename);
    d->map.plr = &d->plr;
    bork_editor_complete_map(&d->map, &ed_map, 1);
    bork_map_init_model(&d->map, &ed_map, core);
    /*  Initialize the player data  */
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = bork_play_update;
    state->tick = bork_play_tick;
    state->draw = bork_play_draw;
    state->deinit = bork_play_deinit;
    pg_mouse_mode(1);
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

static void tick_player(struct bork_play_data* d);
static void tick_held_item(struct bork_play_data* d);
static void tick_datapad(struct bork_play_data* d);
static void tick_control_play(struct bork_play_data* d);
static void tick_menu_base(struct bork_play_data* d);
static void tick_enemies(struct bork_play_data* d);
static void tick_items(struct bork_play_data* d);
static void tick_entities(struct bork_play_data* d);
static void tick_bullets(struct bork_play_data* d);
static void tick_particles(struct bork_play_data* d);
static void tick_fires(struct bork_play_data* d);
static void tick_ambient_sound(struct bork_play_data* d);

void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    ++d->ticks;
    /*  Handle input    */
    if(d->menu.state != BORK_MENU_CLOSED && d->menu.state < BORK_MENU_DOORPAD) {
        tick_menu_base(d);
    }
    if(d->menu.state == BORK_MENU_INVENTORY) tick_control_inv_menu(d);
    else if(d->menu.state == BORK_MENU_UPGRADES) tick_control_upgrade_menu(d);
    else if(d->menu.state == BORK_MENU_RECYCLER) tick_recycler_menu(d);
    else if(d->menu.state == BORK_MENU_DATAPADS) tick_datapad_menu(d);
    else if(d->menu.state == BORK_MENU_GAME) tick_game_menu(d, state);
    else if(d->menu.state == BORK_MENU_DOORPAD) tick_doorpad(d);
    else if(d->menu.state == BORK_MENU_PLAYERDEAD) {
        if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)) {
            struct bork_game_core* core = d->core;
            bork_play_deinit(d);
            bork_menu_start(state, core);
            pg_mouse_mode(0);
            SDL_ShowCursor(SDL_ENABLE);
            pg_flush_input();
            return;
        }
    } else if(d->menu.state == BORK_MENU_CLOSED) {
        ++d->play_ticks;
        if(d->plr.pain_ticks > 0) --d->plr.pain_ticks;
        if(d->teleport_ticks > 0) --d->teleport_ticks;
        vec3 surr_start, surr_end;
        vec3_sub(surr_start, d->plr.pos, (vec3){ 32, 32, 32 });
        vec3_add(surr_end, d->plr.pos, (vec3){ 32, 32, 32 });
        ARR_TRUNCATE(d->plr_enemy_query, 0);
        ARR_TRUNCATE(d->plr_item_query, 0);
        ARR_TRUNCATE(d->plr_entity_query, 0);
        bork_map_query_enemies(&d->map, &d->plr_enemy_query, surr_start, surr_end);
        bork_map_query_items(&d->map, &d->plr_item_query, surr_start, surr_end);
        vec3_sub(surr_start, d->plr.pos, (vec3){ 3, 3, 3 });
        vec3_add(surr_end, d->plr.pos, (vec3){ 3, 3, 3 });
        bork_map_query_entities(&d->map, &d->plr_entity_query, surr_start, surr_end);
        tick_control_play(d);
        tick_held_item(d);
        tick_player(d);
        bork_map_update(&d->map, d);
        tick_upgrades(d);
        tick_bullets(d);
        tick_enemies(d);
        tick_items(d);
        tick_entities(d);
        tick_fires(d);
        tick_particles(d);
        tick_datapad(d);
        tick_ambient_sound(d);
        d->looked_item = get_looked_item(d);
        d->looked_enemy = get_looked_enemy(d);
        d->looked_entity = get_looked_entity(d);
        d->looked_obj = get_looked_map_object(d);
    }
    pg_flush_input();
}

static void reset_menus(struct bork_play_data* d)
{
    d->menu.inv.selection_idx = 0;
    d->menu.inv.scroll_idx = 0;
    d->menu.inv.ammo_select = 0;
    d->menu.upgrades.selection_idx = 0;
    d->menu.upgrades.scroll_idx = 0;
    d->menu.upgrades.horiz_idx = 0;
    d->menu.upgrades.confirm = 0;
    d->menu.upgrades.replace_idx = 0;
    d->menu.recycler.selection_idx = 0;
    d->menu.recycler.scroll_idx = 0;
    d->menu.recycler.obj = NULL;
    d->menu.datapads.selection_idx = 0;
    d->menu.datapads.scroll_idx = 0;
    d->menu.datapads.text_scroll = 0;
    d->menu.datapads.side = 0;
    d->menu.game.selection_idx = 0;
    d->menu.game.save_idx = 0;
}

static void tick_menu_base(struct bork_play_data* d)
{
    float ar = d->core->aspect_ratio;
    vec2 mouse_pos;
    int click;
    pg_mouse_pos(mouse_pos);
    vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0], 1 / d->core->screen_size[1] });
    click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
    if(click) {
        int i;
        for(i = 0; i < 5; ++i) {
            vec2 button_pos = { ar * 0.75 + ((i - 2) * ar * 0.08), 0.125 };
            float dist = vec2_dist(mouse_pos, button_pos);
            if(dist < 0.06) {
                d->menu.state = i + 1;
            }
        }
    }
    if(d->menu.game.mode != GAME_MENU_EDIT_SAVE
    && pg_check_input(SDL_SCANCODE_LSHIFT, PG_CONTROL_HELD)) {
        if(pg_check_input(SDL_SCANCODE_D, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)) {
            ++d->menu.state;
            if(d->menu.state > 5) d->menu.state = 1;
            pg_flush_input();
        } else if(pg_check_input(SDL_SCANCODE_A, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)) {
            --d->menu.state;
            if(d->menu.state == 0) d->menu.state = 5;
            pg_flush_input();
        }
    }
    if(pg_check_gamepad(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PG_CONTROL_HIT)) {
        ++d->menu.state;
        if(d->menu.state > 5) d->menu.state = 1;
        pg_flush_input();
    } else if(pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        --d->menu.state;
        if(d->menu.state == 0) d->menu.state = 5;
        pg_flush_input();
    }
}

static void tick_ambient_sound(struct bork_play_data* d)
{
    pg_audio_set_listener(1, d->plr.pos);
    int i;
    struct bork_sound_emitter* snd;
    ARR_FOREACH_PTR(d->map.sounds, snd, i) {
        float dist = vec3_dist(snd->pos, d->plr.pos);
        if(dist < snd->area) {
            if(snd->handle >= 0) continue;
            else snd->handle = pg_audio_emitter(&d->core->sounds[snd->snd], snd->volume,
                                                snd->area, snd->pos, 1);
        } else if(snd->handle >= 0) {
            pg_audio_emitter_remove(snd->handle);
            snd->handle = -1;
        }
    }
}

static void tick_control_play(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    /*
    if(pg_check_input(SDL_SCANCODE_M, PG_CONTROL_HIT)) {
        if(pg_check_input(SDL_SCANCODE_LSHIFT, PG_CONTROL_HELD)) {
            pg_mouse_mode(0);
            SDL_ShowCursor(SDL_ENABLE);
        } else {
            pg_mouse_mode(1);
            SDL_ShowCursor(SDL_DISABLE);
        }
    }*/
    if(pg_check_input(kmap[BORK_CTRL_MENU], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_START, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_INVENTORY;
        pg_audio_channel_pause(1, 1);
        reset_menus(d);
        SDL_ShowCursor(SDL_ENABLE);
        pg_mouse_mode(0);
    }
    if(pg_check_input(kmap[BORK_CTRL_NEXT_TECH], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        select_next_upgrade(d);
    }
    if(pg_check_input(kmap[BORK_CTRL_DROP], PG_CONTROL_HIT)) {
        drop_item(d);
    }
    if(pg_check_input(kmap[BORK_CTRL_INTERACT], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_X, PG_CONTROL_HIT)) {
        struct bork_entity* item = bork_entity_get(d->looked_item);
        vec3 eye_pos = { d->plr.pos[0], d->plr.pos[1], d->plr.pos[2] };
        if(d->plr.flags & BORK_ENTFLAG_CROUCH) eye_pos[2] += 0.2;
        else eye_pos[2] += 0.8;
        if(item && vec3_dist(item->pos, eye_pos) <= 2.5
        && bork_map_check_vis(&d->map, eye_pos, item->pos)) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
            if(item->flags & BORK_ENTFLAG_IS_AMMO) {
                int ammo_type = item->type - BORK_ITEM_BULLETS;
                d->ammo[ammo_type] += prof->ammo_capacity;
                item->flags |= BORK_ENTFLAG_DEAD;
            } else if(item->type == BORK_ITEM_DATAPAD) {
                d->hud_datapad_id = item->counter[0];
                d->hud_datapad_ticks = 5 * 60;
                d->hud_datapad_line = 0;
                d->held_datapads[d->num_held_datapads++] = item->counter[0];
                item->flags |= BORK_ENTFLAG_DEAD;
            } else if(item->type == BORK_ITEM_UPGRADE) {
                item->flags |= BORK_ENTFLAG_IN_INVENTORY;
                ARR_PUSH(d->held_upgrades, d->looked_item);
            } else if(item->type == BORK_ITEM_SCHEMATIC) {
                item->flags |= BORK_ENTFLAG_DEAD;
                d->held_schematics |= (1 << item->counter[0]);
            } else {
                item->flags |= BORK_ENTFLAG_IN_INVENTORY;
                add_inventory_item(d, d->looked_item);
            }
            pg_audio_play(&d->core->sounds[BORK_SND_PICKUP], 0.5);
        } else if(d->looked_obj) {
            if(d->looked_obj->type == BORK_MAP_DOORPAD) {
                struct bork_map_object* door = &d->map.doors.data[d->looked_obj->doorpad.door_idx];
                if(door->door.locked == 1) {
                    d->menu.doorpad.unlocked_ticks = 0;
                    d->menu.doorpad.selection[0] = -1;
                    d->menu.doorpad.selection[1] = 0;
                    d->menu.doorpad.num_chars = 0;
                    d->menu.doorpad.door_idx = d->looked_obj->doorpad.door_idx;
                    d->menu.state = BORK_MENU_DOORPAD;
                    SDL_ShowCursor(SDL_ENABLE);
                    pg_mouse_mode(0);
                    pg_audio_channel_pause(1, 1);
                } else {
                    if(!door->door.open) {
                        pg_audio_play(&d->core->sounds[BORK_SND_DOOR_OPEN], 0.5);
                    } else {
                        pg_audio_play(&d->core->sounds[BORK_SND_DOOR_CLOSE], 0.5);
                    }
                    door->door.open = 1 - door->door.open;
                }
            } else if(d->looked_obj->type == BORK_MAP_RECYCLER) {
                reset_menus(d);
                d->menu.state = BORK_MENU_RECYCLER;
                d->menu.recycler.obj = d->looked_obj;
                SDL_ShowCursor(SDL_ENABLE);
                pg_mouse_mode(0);
                pg_audio_channel_pause(1, 1);
            }
        }
    }
    if(d->held_item >= 0
    && (pg_check_input(kmap[BORK_CTRL_RELOAD], PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PG_CONTROL_HIT))) {
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
        d->jump_released = 0;
        pg_audio_play(&d->core->sounds[BORK_SND_JUMP], 0.1);
    }
    if(pg_check_input(kmap[BORK_CTRL_JUMP], PG_CONTROL_RELEASED)) {
        d->jump_released = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_CROUCH], PG_CONTROL_HELD)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSTICK, PG_CONTROL_HELD)) {
        d->plr.flags |= BORK_ENTFLAG_CROUCH;
    } else if(d->plr.flags & BORK_ENTFLAG_CROUCH) {
        vec3 check_pos;
        vec3_sub(check_pos, d->plr.pos, (vec3){ 0, 0, 0.1 });
        if(bork_map_vis_dist(&d->map, check_pos, (vec3){ 0, 0, 1 }, 1) >= 0.75) {
            d->plr.flags &= ~BORK_ENTFLAG_CROUCH;
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
    if(vec2_len(d->plr.vel) > 0.1) vec2_set_len(d->plr.vel, d->plr.vel, 0.1);
    if((d->plr.flags & BORK_ENTFLAG_GROUND) && vec2_len(d->plr.vel) > 0.01) {
        if(d->walk_input_ticks % 90 == 10)
            pg_audio_play(&d->core->sounds[BORK_SND_FOOTSTEP1], 0.1);
        else if(d->walk_input_ticks % 90 == 55)
            pg_audio_play(&d->core->sounds[BORK_SND_FOOTSTEP2], 0.1);
    }
    if(!kbd_input) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(stick[0] != 0 || stick[1] != 0) {
            kbd_input = 1;
            float c = cos(-d->plr.dir[0]), s = sin(d->plr.dir[0]);
            vec2 stick_rot = {
                stick[0] * s - stick[1] * c,
                stick[0] * c + stick[1] * s };
            vec2_normalize(stick_rot, stick_rot);
            if(vec2_len(stick_rot) > 0.5) kbd_input = 1;
            d->plr.vel[0] += move_speed * stick_rot[0];
            d->plr.vel[1] -= move_speed * stick_rot[1];
        }
    }
    if(kbd_input) ++d->walk_input_ticks;
    else d->walk_input_ticks = 0;
    if((d->plr.flags & BORK_ENTFLAG_GROUND) && vec2_len(d->plr.vel) > 0.01) {
        if(d->walk_input_ticks % 90 == 10)
            pg_audio_play(&d->core->sounds[BORK_SND_FOOTSTEP1], 0.1);
        else if(d->walk_input_ticks % 90 == 55)
            pg_audio_play(&d->core->sounds[BORK_SND_FOOTSTEP2], 0.1);
    }
    if(pg_check_input(kmap[BORK_CTRL_FLASHLIGHT], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_Y, PG_CONTROL_HIT)) {
        d->flashlight_on = 1 - d->flashlight_on;
    }
    if(d->held_item != -1) {
        bork_entity_t held_id = d->inventory.data[d->held_item];
        struct bork_entity* held_ent = bork_entity_get(held_id);
        if(held_ent) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[held_ent->type];
            if(!d->reload_ticks && prof->use_ctrl
            && (pg_check_input(kmap[BORK_CTRL_FIRE], prof->use_ctrl)
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
    /*
    if(d->play_ticks % 10 == 0) {
        float dist_ctr = 1 - (vec2_dist(d->plr.pos, (vec2){ 32, 32 }) / 16);
        float loop_time = 10.0f / 120.0f;
        float loop_origin = d->play_ticks / 120.0f;
        pg_audio_loop(&d->core->sounds[BORK_SND_FIRE], dist_ctr * 0.25, loop_origin, loop_time);
    }*/
}

static void tick_player(struct bork_play_data* d)
{
    int plr_x = floor(d->plr.pos[0] / 2);
    int plr_y = floor(d->plr.pos[1] / 2);
    int plr_z = floor(d->plr.pos[2] / 2);
    if((plr_x != d->plr_map_pos[0] || plr_y != d->plr_map_pos[1]
    || plr_z != d->plr_map_pos[2])) {
        struct bork_tile* tile = bork_map_tile_ptri(&d->map, plr_x, plr_y, plr_z);
        if(tile && tile->type == BORK_TILE_EDITOR_TELEPORT) {
            int i;
            struct bork_map_object* obj;
            ARR_FOREACH_PTR(d->map.teleports, obj, i) {
                if((int)(obj->pos[0] / 2) == plr_x && (int)(obj->pos[1] / 2) == plr_y
                && (int)(obj->pos[2] / 2) == plr_z) {
                    break;
                }
            }
            int j;
            struct bork_map_object* match;
            ARR_FOREACH_PTR(d->map.teleports, match, j) {
                if(match != obj && match->teleport.id == obj->teleport.id) {
                    vec3_dup(d->plr.pos, match->pos);
                    vec3_set(d->plr.vel, 0, 0, 0);
                    d->plr.dir[0] = match->teleport.dir;
                    d->teleport_ticks = PLAY_SECONDS(2);
                    pg_audio_play(&d->core->sounds[BORK_SND_TELEPORT], 0.5);
                }
            }
        }
        plr_x = floor(d->plr.pos[0] / 2);
        plr_y = floor(d->plr.pos[1] / 2);
        plr_z = floor(d->plr.pos[2] / 2);
        d->plr_map_pos[0] = plr_x;
        d->plr_map_pos[1] = plr_y;
        d->plr_map_pos[2] = plr_z;
        if(!d->decoy_active) bork_map_build_plr_dist(&d->map, d->plr.pos);
    }
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(d->plr_entity_query, ent_id, i) {
        surr_ent = bork_entity_get(ent_id);
        if(!surr_ent) continue;
        const struct bork_entity_profile* surr_prof = &BORK_ENT_PROFILES[surr_ent->type];
        vec3 push;
        vec3_sub(push, d->plr.pos, surr_ent->pos);
        float full = 0.5 + surr_prof->size[0];
        float dist = vec3_len(push);
        if(dist < full) {
            vec3_set_len(push, push, (full - dist));
            vec2_add(d->plr.vel, d->plr.vel, push);
            vec3_sub(surr_ent->vel, surr_ent->vel, push);
        }
    }
    if(d->plr.flags & BORK_ENTFLAG_ON_FIRE) {
        entity_on_fire(d, &d->plr);
        if(d->play_ticks % 10 == 0) {
            int fire_ticks = d->plr.fire_ticks;
            float vol = 2.5;
            if(fire_ticks < 120) vol *= (float)fire_ticks / 120;
            float loop_time = 10.0f / 120.0f;
            float loop_origin = (float)d->play_ticks / 120.0f;
            pg_audio_loop(&d->core->sounds[BORK_SND_FIRE], vol, loop_origin, loop_time);
        }
    }
    vec2_set(d->plr.dir, d->plr.dir[0] + d->mouse_motion[0],
                         d->plr.dir[1] + d->mouse_motion[1]);
    vec2_set(d->mouse_motion, 0, 0);
    d->plr.dir[0] = fmodf(d->plr.dir[0], M_PI * 2.0f);
    bork_entity_update(&d->plr, d);
    if(d->plr.HP <= 0) d->menu.state = BORK_MENU_PLAYERDEAD;
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
                --i;
                continue;
            }
            if(ent->last_tick == d->ticks) continue;
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
            if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                ent->last_tick = d->ticks;
                if(prof->tick_func) prof->tick_func(ent, d);
                ent = bork_entity_get(ent_id);
                if(!ent) {
                    ARR_SWAPSPLICE(d->map.enemies[x][y][z], i, 1);
                    --i;
                    continue;
                }
            } else {
                ARR_SWAPSPLICE(d->map.enemies[x][y][z], i, 1);
                --i;
                bork_map_add_enemy(&d->map, ent_id);
            }
            if(d->play_ticks % 30 == 0) {
                int j;
                struct bork_fire* fire;
                ARR_FOREACH_PTR(d->map.fires, fire, j) {
                    if(vec2_dist(fire->pos, ent->pos) < 1
                    && fire->pos[2] < ent->pos[2] && ent->pos[2] - fire->pos[2] < 2) {
                        create_sparks(d, ent->pos, 0.1, 3);
                        ent->HP -= 5;
                        if(rand() % 4 == 0) {
                            ent->flags |= BORK_ENTFLAG_ON_FIRE;
                            ent->fire_ticks = 600;
                        }
                        break;
                    }
                }
            }
        }
    }
}

static void tick_entities(struct bork_play_data* d)
{
    static bork_entity_arr_t surr = {};
    int x, y, z, i;
    for(x = 0; x < 4; ++x)
    for(y = 0; y < 4; ++y)
    for(z = 0; z < 4; ++z) {
        bork_entity_t ent_id;
        struct bork_entity* ent;
        ARR_FOREACH_REV(d->map.entities[x][y][z], ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent || (ent->flags & BORK_ENTFLAG_DEAD)) {
                ARR_SWAPSPLICE(d->map.entities[x][y][z], i, 1);
                continue;
            }
            if(ent->last_tick == d->ticks) continue;
            else if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
                ent->last_tick = d->ticks;
                bork_entity_update(ent, d);
                ARR_TRUNCATE(surr, 0);
                vec3 start, end;
                vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
                vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
                bork_map_query_entities(&d->map, &surr, start, end);
                /*  Do basic physics    */
                bork_entity_move(ent, &d->map);
                /*  Physics against other enemies   */
                int j;
                struct bork_entity* surr_ent;
                bork_entity_t surr_id;
                ARR_FOREACH(surr, surr_id, j) {
                    surr_ent = bork_entity_get(surr_id);
                    if(!surr_ent) continue;
                    const struct bork_entity_profile* surr_prof = &BORK_ENT_PROFILES[surr_ent->type];
                    vec3 push;
                    vec3_sub(push, ent->pos, surr_ent->pos);
                    float full = prof->size[0] + surr_prof->size[0];
                    float dist = vec3_len(push);
                    if(dist < full) {
                        vec3_set_len(push, push, (full - dist) * 0.5);
                        vec3_add(ent->vel, ent->vel, push);
                    }
                }
                if(ent->flags & BORK_ENTFLAG_ACTIVE_FUNC) {
                    prof->tick_func(ent, d);
                }
                if(ent->HP <= 0) {
                    if(prof->death_func) prof->death_func(ent, d);
                    else ent->flags |= BORK_ENTFLAG_DEAD;
                }
            } else {
                ARR_SWAPSPLICE(d->map.entities[x][y][z], i, 1);
                --i;
                bork_map_add_entity(&d->map, ent_id);
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
    bork_entity_update(ent, d);
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
                --i;
                continue;
            }
            if(ent->last_tick == d->ticks) continue;
            else if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                ent->last_tick = d->ticks;
                tick_item(d, ent);
                if((ent->flags & BORK_ENTFLAG_ITEM) && (ent->flags & BORK_ENTFLAG_ACTIVE_FUNC)) {
                    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
                    prof->tick_func(ent, d);
                }
            } else {
                ARR_SWAPSPLICE(d->map.items[x][y][z], i, 1);
                --i;
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
            bork_bullet_move(blt, d);
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
        if(part->flags & BORK_PARTICLE_GRAVITY) part->vel[2] -= 0.005;
        else if(part->flags & BORK_PARTICLE_BOUYANT) part->vel[2] += 0.00025;
        if(part->flags & BORK_PARTICLE_DECELERATE) vec3_scale(part->vel, part->vel, 0.95);
        if(part->flags & BORK_PARTICLE_COLLIDE_DIE) {
            if(bork_map_check_sphere(&d->map, NULL, part->pos, 0.1)) {
                part->ticks_left = 0;
            }
        }

    }
}

static void tick_fires(struct bork_play_data* d)
{
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    int fire_damage = 8;
    int plr_heatshield_lvl = get_upgrade_level(d, BORK_UPGRADE_HEATSHIELD);
    if(plr_heatshield_lvl == 0) fire_damage = 4;
    else if(plr_heatshield_lvl == 1) fire_damage = 0;
    struct bork_fire* fire;
    int i;
    ARR_FOREACH_REV_PTR(d->map.fires, fire, i) {
        --fire->lifetime;
        float dist = vec3_dist(fire->pos, d->plr.pos);
        if(dist > 32) {
            continue;
        }
        if(fire->audio_handle == -1 && !(fire->flags & BORK_FIRE_MOVES)) {
            fire->audio_handle =
                pg_audio_emitter(&d->core->sounds[BORK_SND_FIRE], 1, 6, fire->pos, 1);
        }
        if(fire->flags & BORK_FIRE_MOVES) {
            vec3_add(fire->pos, fire->pos, fire->vel);
            fire->vel[2] -= 0.005;
            struct bork_collision coll;
            int hit = bork_map_collide(&d->map, &coll, fire->pos, (vec3){ 0.3, 0.3, 0.3 });
            if(hit) {
                fire->audio_handle =
                    pg_audio_emitter(&d->core->sounds[BORK_SND_FIRE], 1, 6, fire->pos, 2);
                vec3_set(fire->vel, 0, 0, 0);
                fire->flags &= ~BORK_FIRE_MOVES;
            }
        }
        if(fire->lifetime % 41 == 0) {
            vec3 off = { ((float)rand() / RAND_MAX - 0.5) * 0.5,
                         ((float)rand() / RAND_MAX - 0.5) * 0.5,
                         ((float)rand() / RAND_MAX) * 0.5 };
            vec3_add(off, off, fire->pos);
            create_smoke(d, off, (vec3){}, 180);
        } else if(fire->lifetime % 31 == 0) {
            vec3 off = { ((float)rand() / RAND_MAX - 0.5) * 0.5,
                         ((float)rand() / RAND_MAX - 0.5) * 0.5,
                         ((float)rand() / RAND_MAX) * 0.5 };
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT | BORK_PARTICLE_DECELERATE,
                .pos = { fire->pos[0] + off[0], fire->pos[1] + off[1], fire->pos[2] + off[2] },
                .vel = { 0, 0, -0.005 },
                .ticks_left = 120,
                .frame_ticks = 0,
                .current_frame = 24 + rand() % 4,
            };
            ARR_PUSH(d->particles, new_part);
        }
        if(d->play_ticks % 30 == 0) {
            ARR_TRUNCATE(surr, 0);
            vec3 start, end;
            vec3_sub(start, fire->pos, (vec3){ 2, 2, 2 });
            vec3_add(end, fire->pos, (vec3){ 2, 2, 2 });
            bork_map_query_enemies(&d->map, &surr, start, end);
            int j;
            bork_entity_t surr_ent_id;
            struct bork_entity* surr_ent;
            ARR_FOREACH(surr, surr_ent_id, j) {
                surr_ent = bork_entity_get(surr_ent_id);
                if(!surr_ent || surr_ent->last_fire_tick == d->play_ticks) continue;
                surr_ent->last_fire_tick = d->play_ticks;
                surr_ent->HP -= 5;
                if(rand() % 3 == 0) {
                    surr_ent->flags |= BORK_ENTFLAG_ON_FIRE;
                    surr_ent->fire_ticks = PLAY_SECONDS(2.5);
                }
            }
            if(plr_heatshield_lvl < 1 && d->plr.last_fire_tick != d->play_ticks
            && vec2_dist(fire->pos, d->plr.pos) < 1
            && fabs((fire->pos[2] + 1.5) - d->plr.pos[2]) < 2) {
                d->plr.last_fire_tick = d->play_ticks;
                d->plr.HP -= fire_damage;
                d->plr.pain_ticks += 30;
                if(plr_heatshield_lvl < 0 && rand() % 3 == 0) {
                    d->plr.flags |= BORK_ENTFLAG_ON_FIRE;
                    d->plr.fire_ticks = PLAY_SECONDS(5) + PLAY_SECONDS(RANDF * 2);
                }
            }
        }
        if(fire->lifetime == 0) {
            if(fire->audio_handle >= 0) {
                pg_audio_emitter_remove(fire->audio_handle);
            }
            ARR_SWAPSPLICE(d->map.fires, i, 1);
        }
    }
}

/*  Drawing */
static void draw_hud_overlay(struct bork_play_data* d);
static void draw_datapad(struct bork_play_data* d);
static void draw_gameover(struct bork_play_data* d, float t);
static void draw_weapon(struct bork_play_data* d, float hud_anim_lerp,
                        vec3 pos_lerp, vec2 dir_lerp);
static void draw_decoy(struct bork_play_data* d);
static void draw_enemies(struct bork_play_data* d, float lerp);
static void draw_items(struct bork_play_data* d, float lerp);
static void draw_entities(struct bork_play_data* d, float lerp);
static void draw_bullets(struct bork_play_data* d, float lerp);
static void draw_map_lights(struct bork_play_data* d);
static void draw_lights(struct bork_play_data* d);
static void draw_particles(struct bork_play_data* d);
static void draw_fires(struct bork_play_data* d);

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
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    pg_shader_begin(&d->core->shader_3d, &d->core->view);
    draw_weapon(d, d->hud_anim_progress + (d->hud_anim_speed * state->tick_over),
                draw_pos, draw_dir);
    pg_shader_begin(&d->core->shader_sprite, &d->core->view);
    draw_enemies(d, t);
    if(d->decoy_active) draw_decoy(d);
    draw_items(d, t);
    draw_entities(d, t);
    draw_bullets(d, t);
    draw_fires(d);
    if(d->plr.flags & BORK_ENTFLAG_ON_FIRE) {
        struct pg_light new_light = {};
        pg_light_pointlight(&new_light, draw_pos, 2.5, (vec3){ 2.0, 1.5, 0 });
        ARR_PUSH(d->lights_buf, new_light);
    }
    draw_particles(d);
    pg_shader_3d_texture(&d->core->shader_3d, &d->core->env_atlas);
    bork_map_draw(&d->map, d);
    /*  Test 3d text    */
    pg_shader_begin(&d->core->shader_text, NULL);
    pg_shader_text_3d(&d->core->shader_text, &d->core->view);
    mat4 text_tx;
    mat4_translate(text_tx, 8, 46, 5.5);
    mat4_rotate_X(text_tx, text_tx, M_PI * -0.5);
    mat4_rotate_X(text_tx, text_tx, (float)d->ticks / 120);
    pg_shader_text_transform_3d(&d->core->shader_text, text_tx);
    struct pg_shader_text test_text = {
        .use_blocks = 2,
        .block = { "3D TEXT!", "ANOTHER LINE" },
        .block_style = { { 0, -0.5, 1, 1.2 }, { 0, 0.6, 1, 1.2 }},
        .block_color = { { 1, 1, 1, 0.5 }, { 1, 1, 1, 0.5 }} };
    pg_shader_text_write(&d->core->shader_text, &test_text);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /*  End text    */
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
        pg_light_spotlight(&flashlight, flash_pos, 12, (vec3){ 1, 1, 1 },
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
        bork_draw_backdrop(d->core, (vec4){ 1.5, 1.5, 1.5, 0.75 },
                           (float)state->ticks / (float)state->tick_rate);
        bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 0.9 });
        pg_shader_begin(&d->core->shader_2d, NULL);
        if(d->menu.state < BORK_MENU_DOORPAD) draw_active_menu(d);
        switch(d->menu.state) {
        default: break;
        case BORK_MENU_INVENTORY:
            draw_menu_inv(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_UPGRADES:
            draw_upgrade_menu(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_RECYCLER:
            draw_recycler_menu(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_DATAPADS:
            draw_datapad_menu(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_GAME:
            draw_game_menu(d, (float)state->ticks / (float)state->tick_rate);
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
    vec3 plr_eye;
    bork_entity_get_eye(&d->plr, NULL, plr_eye);
    int i;
    int nearlights = 0;
    struct pg_light* light;
    ARR_FOREACH_PTR(d->lights_buf, light, i) {
        vec3 light_to_plr;
        vec3_sub(light_to_plr, light->pos, plr_eye);
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
        vec3_sub(light_to_plr, light->pos, plr_eye);
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

void draw_active_menu(struct bork_play_data* d)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    vec2 mouse;
    pg_mouse_pos(mouse);
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    int i;
    for(i = 0; i < 5; ++i) {
        int active = ((d->menu.state - 1) == i);
        if(d->menu.recycler.obj && i == BORK_MENU_RECYCLER - 1) {
            pg_shader_2d_color_mod(shader, (vec4){ 0.3, 0.3, 0.9, 1 }, (vec4){});
        } else if(active) pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
        else pg_shader_2d_color_mod(shader, (vec4){ 0.5, 0.5, 0.5, 1 }, (vec4){});
        pg_shader_2d_tex_frame(shader, 166 + i * 2);
        pg_shader_2d_add_tex_tx(shader, (vec2){ 2, 2 }, (vec2){ 0, 0 });
        pg_shader_2d_transform(shader, (vec2){ ar * 0.75 + ((i - 2) * ar * 0.08), 0.125 },
                               (vec2){ 0.06, 0.06 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
}

static void draw_hud_overlay(struct bork_play_data* d)
{
    float ar = d->core->aspect_ratio;
    pg_shader_begin(&d->core->shader_text, NULL);
    pg_shader_text_resolution(&d->core->shader_text, (vec2){ ar, 1 });
    pg_shader_text_transform(&d->core->shader_text, (vec2){ 1, 1 }, (vec2){});
    vec2 screen_pos;
    enum bork_resource looked_resource;
    int looking_at_resource = 0;
    float name_center = 0;
    struct bork_entity* looked_ent;
    int ti = 0;
    int len;
    struct pg_shader_text text = { .use_blocks = 0 };
    if(d->looked_item != -1 && (looked_ent = bork_entity_get(d->looked_item))) {
        if(looked_ent->flags & BORK_ENTFLAG_IS_FOOD) {
            looking_at_resource = 1;
            looked_resource = BORK_RESOURCE_FOODSTUFF;
        } else if(looked_ent->flags & BORK_ENTFLAG_IS_CHEMICAL) {
            looking_at_resource = 1;
            looked_resource = BORK_RESOURCE_CHEMICAL;
        } else if(looked_ent->flags & BORK_ENTFLAG_IS_ELECTRICAL) {
            looking_at_resource = 1;
            looked_resource = BORK_RESOURCE_ELECTRICAL;
        } else if(looked_ent->flags & BORK_ENTFLAG_IS_RAW_MATERIAL) {
            looking_at_resource = 1;
            looked_resource = BORK_RESOURCE_SCRAP;
        }
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[looked_ent->type];
        float dist = vec3_dist(d->plr.pos, looked_ent->pos);
        pg_viewer_project(&d->core->view, screen_pos, looked_ent->pos);
        ti = 0;
        len = snprintf(text.block[++ti], 64, "%s", prof->name);
        name_center = ((float)len * 0.04 * 1.25 * 0.5);
        vec2_add(screen_pos, screen_pos, (vec2){ 0, -0.35 / MAX(dist,1) });
        vec4_set(text.block_style[ti], screen_pos[0] - name_center, screen_pos[1], 0.04, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        if(looked_ent->type == BORK_ITEM_SCHEMATIC) {
            const struct bork_schematic_detail* sch_d =
                &BORK_SCHEMATIC_DETAIL[looked_ent->counter[0]];
            const struct bork_entity_profile* sch_prof =
                &BORK_ENT_PROFILES[sch_d->product];
            len = snprintf(text.block[++ti], 64, "%s", sch_prof->name);
            float sch_center = (float)len * 0.04 * 1.25 * 0.5;
            vec4_set(text.block_style[ti], screen_pos[0] - sch_center, screen_pos[1] - 0.05, 0.04, 1.25);
            vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        }
    }
    if(d->scan_ticks) {
        len = snprintf(text.block[++ti], 64, "%s", d->scanned_name);
        vec4_set(text.block_style[ti], 0.2 - (len * 0.02 * 1.25 * 0.5), 0.6, 0.02, 1.25);
        float scan_alpha;
        if(d->scan_ticks > PLAY_SECONDS(4.75)) {
            scan_alpha = PLAY_SECONDS(5) - d->scan_ticks;
            scan_alpha /= PLAY_SECONDS(0.25);
        } else if(d->scan_ticks < PLAY_SECONDS(0.25)) {
            scan_alpha = d->scan_ticks;
            scan_alpha /= PLAY_SECONDS(0.25);
        } else scan_alpha = 1;
        vec4_set(text.block_color[ti], 1, 1, 1, scan_alpha);
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(&d->core->shader_text, &text);
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
    pg_model_begin(&d->core->quad_2d_ctr, &d->core->shader_2d);
    pg_shader_2d_resolution(&d->core->shader_2d, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    if(looking_at_resource) {
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.75 }, (vec4){});
        pg_shader_2d_tex_frame(&d->core->shader_2d, 200 + looked_resource * 2);
        pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ 2, 2 }, (vec2){});
        pg_shader_2d_transform(&d->core->shader_2d,
            (vec2){ screen_pos[0], screen_pos[1] - 0.075 },
            (vec2){ 0.05, 0.05 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_model_begin(&d->core->quad_2d_ctr, &d->core->shader_2d);
    pg_shader_2d_tex_frame(&d->core->shader_2d, 214);
    pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ 2, 2 }, (vec2){});
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.5 * ar, 0.5 }, (vec2){ 0.025, 0.025 }, 0);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.5 }, (vec4){});
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_model_begin(&d->core->quad_2d, &d->core->shader_2d);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_shader_2d_tex_frame(&d->core->shader_2d, 232);
    float hp_frac = (float)d->plr.HP / 100.0f;
    float hp_offset = (1 - hp_frac) * (24.0f / 256.0f);
    pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ hp_frac * 3, 2 }, (vec2){ hp_offset, 0 });
    pg_shader_2d_transform(&d->core->shader_2d,
                           (vec2){ 0.05 + 0.15 * (1 - hp_frac), 0.75 },
                           (vec2){ 0.3 * hp_frac, 0.2 }, 0);
    if(hp_frac < 0.5) {
        float red_frac = (hp_frac / 0.5);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, red_frac, red_frac, 1 }, (vec4){});
    }
    pg_model_draw(&d->core->quad_2d, NULL);
    if(d->teleport_ticks > 0) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){}, (vec2){ ar, 1 }, 0);
        pg_shader_2d_texture(&d->core->shader_2d, &d->core->radial_vignette);
        float hurt_alpha = (float)d->teleport_ticks / 240 * 0.5;
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 0, 0.5, 0, hurt_alpha }, (vec4){});
        pg_model_draw(&d->core->quad_2d, NULL);
    } else if(d->plr.pain_ticks > 0) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){}, (vec2){ ar, 1 }, 0);
        pg_shader_2d_texture(&d->core->shader_2d, &d->core->radial_vignette);
        float hurt_alpha = (float)d->plr.pain_ticks / 120 * 0.5;
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 0.5, 0, 0, hurt_alpha }, (vec4){});
        pg_model_draw(&d->core->quad_2d, NULL);
    }
    draw_upgrade_hud(d);
}

static void draw_datapad(struct bork_play_data* d)
{
    float ar = d->core->aspect_ratio;
    if(d->hud_datapad_id < 0) return;
    const struct bork_datapad* dp = &BORK_DATAPADS[d->hud_datapad_id];
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_shader_2d_set_light(shader, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_begin(shader, NULL);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    pg_shader_2d_tex_frame(shader, 66);
    pg_shader_2d_transform(shader, (vec2){ ar * 0.5, 0.84 }, (vec2){ 0.1, 0.1 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_tex_frame(shader, 236);
    pg_shader_2d_add_tex_tx(shader, (vec2){ 4, 1.5 }, (vec2){});
    pg_shader_2d_transform(shader, (vec2){ ar * 0.5, 0.75 }, (vec2){ 0.25, 0.1 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = { .use_blocks = 2 };
    int len = snprintf(text.block[0], 64, "%s", dp->title);
    vec4_set(text.block_style[0], ar * 0.5 - (len * 0.025 * 1.125 * 0.5), 0.95, 0.025, 1.125);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    len = snprintf(text.block[1], 64, "%s", dp->text[d->hud_datapad_line]);
    vec4_set(text.block_style[1], ar * 0.5 - (len * 0.025 * 1.125 * 0.5), 0.6, 0.025, 1.125);
    vec4_set(text.block_color[1], 1, 1, 1, 1);
    if(d->hud_datapad_line < dp->lines - 1) {
        text.use_blocks = 3;
        len = snprintf(text.block[2], 64, "%s", dp->text[d->hud_datapad_line + 1]);
        vec4_set(text.block_style[2], ar * 0.5 - (len * 0.025 * 1.125 * 0.5), 0.63, 0.025, 1.125);
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

static void draw_decoy(struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_texture(shader, &d->core->item_tex);
    pg_shader_sprite_tex_frame(shader, 224);
    pg_shader_sprite_add_tex_tx(shader, (vec2){ 1.5, 1.5 }, (vec2){});
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){});
    pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(&d->core->enemy_model, shader);
    mat4 transform;
    mat4_translate(transform, d->decoy_pos[0], d->decoy_pos[1], d->decoy_pos[2]);
    pg_model_draw(&d->core->enemy_model, transform);
    struct pg_light new_light = {};
    pg_light_pointlight(&new_light, d->decoy_pos, 2.5, (vec3){ 2, 2, 2 });
    ARR_PUSH(d->lights_buf, new_light);
}

static void draw_enemies(struct bork_play_data* d, float lerp)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->enemy_model;
    int current_frame = 0;
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_texture(shader, &d->core->enemies_tex);
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
            if(ent->flags & BORK_ENTFLAG_ON_FIRE) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light, pos_lerp, 2.5, (vec3){ 2.0, 1.5, 0 });
                ARR_PUSH(d->lights_buf, new_light);
            }
            vec2 ent_to_plr;
            vec2_sub(ent_to_plr, pos_lerp, d->plr.pos);
            float dir_adjust = 1.0f / (float)prof->dir_frames * 0.5;
            float angle = M_PI - atan2(ent_to_plr[0], ent_to_plr[1]) - ent->dir[0];
            angle = FMOD(angle, M_PI * 2);
            float angle_f = angle / (M_PI * 2) - dir_adjust;
            angle_f = 1.0f - FMOD(angle_f, 1.0f);
            int frame = (int)(angle_f * (float)prof->dir_frames);
            if(ent->type == BORK_ENEMY_GREAT_BANE) frame *= 2;
            frame += prof->front_frame;
            if(frame != current_frame) {
                pg_shader_sprite_tex_frame(shader, frame);
                pg_shader_sprite_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
                pg_shader_sprite_transform(shader,
                    (vec2){ prof->sprite_tx[0], prof->sprite_tx[1] },
                    prof->sprite_tx + 2);
                current_frame = frame;
            }
            /*  Draw the sprite */
            mat4 transform;
            mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2]);
            pg_model_draw(model, transform);
            if(ent->type == BORK_ENEMY_FANG_BANGER
            && !(ent->flags & BORK_ENTFLAG_EMP)) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light,
                    (vec3){ pos_lerp[0], pos_lerp[1], pos_lerp[2] + 0.5 },
                    2, (vec3){ 0.9, 0.5, 0.5 });
                ARR_PUSH(d->lights_buf, new_light);
            }
        }
    }
}

static void draw_entities(struct bork_play_data* d, float lerp)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->bullet_model;
    int current_frame = -1;
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_texture(shader, &d->core->item_tex);
    pg_shader_sprite_color_mod(shader, (vec4){ 1.0f, 1.0f, 1.0f, 1.0f });
    pg_model_begin(model, shader);
    int x, y, z, i;
    for(x = 0; x < 4; ++x)
    for(y = 0; y < 4; ++y)
    for(z = 0; z < 4; ++z) {
        bork_entity_t ent_id;
        struct bork_entity* ent;
        const struct bork_entity_profile* prof;
        ARR_FOREACH(d->map.entities[x][y][z], ent_id, i) {
            ent = bork_entity_get(ent_id);
            if(!ent) continue;
            prof = &BORK_ENT_PROFILES[ent->type];
            if(ent->flags & BORK_ENTFLAG_DEAD) continue;
            if(prof->front_frame != current_frame) {
                current_frame = prof->front_frame;
                pg_shader_sprite_tex_frame(shader, prof->front_frame);
                pg_shader_sprite_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
                pg_shader_sprite_transform(shader,
                    (vec2){ prof->sprite_tx[0] * 1.5, prof->sprite_tx[1] * 1.5 },
                    prof->sprite_tx + 2);
            }
            vec3 pos_lerp;
            vec3_scale(pos_lerp, ent->vel, lerp);
            vec3_add(pos_lerp, pos_lerp, ent->pos);
            mat4 transform;
            mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2] + 0.05);
            pg_model_draw(model, transform);
            if(ent->type == BORK_ENTITY_BARREL) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light,
                    (vec3){ pos_lerp[0], pos_lerp[1], pos_lerp[2] + 0.75 }, 1.5,
                    (vec3){ 0.0, 0.1, 0.0 });
                ARR_PUSH(d->lights_buf, new_light);
            }
        }
    }
}

static void draw_items(struct bork_play_data* d, float lerp)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->bullet_model;
    int current_frame = -1;
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_texture(shader, &d->core->item_tex);
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
                pg_shader_sprite_transform(shader,
                    (vec2){ prof->sprite_tx[0] * 1.5, prof->sprite_tx[1] * 1.5 },
                    prof->sprite_tx + 2);
            }
            vec3 pos_lerp;
            vec3_scale(pos_lerp, ent->vel, lerp);
            vec3_add(pos_lerp, pos_lerp, ent->pos);
            mat4 transform;
            mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2] + 0.05);
            if(ent->type == BORK_ITEM_DESKLAMP) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light, pos_lerp, 2, (vec3){ 0.9, 0.5, 0.5 });
                ARR_PUSH(d->lights_buf, new_light);
            }
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
        if(vec3_dist2(light->pos, d->plr.pos) > (32 * 32)) continue;
        ARR_PUSH(d->lights_buf, *light);
    }
    ARR_FOREACH_PTR(d->map.spotlights, light, i) {
        if(vec3_dist2(light->pos, d->plr.pos) > (32 * 32)) continue;
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
        if(vec3_dist2(fire->pos, d->plr.pos) > (24 * 24)) continue;
        vec3_dup(light.pos, fire->pos);
        ARR_PUSH(d->lights_buf, light);
    }
    struct bork_map_object* obj;
    ARR_FOREACH_PTR(d->map.fire_objs, obj, i) {
        if(vec3_dist2(obj->pos, d->plr.pos) > (24 * 24)) continue;
        vec3_dup(light.pos, obj->pos);
        light.size = 3;
        ARR_PUSH(d->lights_buf, light);
    }
}

static void draw_particles(struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_sprite;
    struct pg_model* model = &d->core->enemy_model;
    pg_shader_sprite_transform(shader, (vec2){ 0.75, 0.75 }, (vec2){ 0, 0 });
    pg_shader_sprite_texture(shader, &d->core->particle_tex);
    pg_shader_sprite_tex_frame(shader, 0);
    pg_shader_sprite_mode(shader, PG_SPRITE_SPHERICAL);
    pg_shader_sprite_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    pg_model_begin(model, shader);
    mat4 part_transform;
    struct bork_particle* part;
    int i;
    ARR_FOREACH_PTR(d->particles, part, i) {
        if(part->flags & BORK_PARTICLE_SPRITE) {
            pg_shader_sprite_tex_frame(shader, part->current_frame);
            mat4_translate(part_transform, part->pos[0], part->pos[1], part->pos[2]);
            pg_model_draw(model, part_transform);
        }
        if(part->flags & BORK_PARTICLE_LIGHT) {
            struct pg_light light;
            pg_light_pointlight(&light, part->pos,
                (float)part->ticks_left / (float)part->lifetime * part->light[3], part->light);
            ARR_PUSH(d->lights_buf, light);
        }
    }
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

static size_t write_entity(FILE* f, struct bork_entity* ent)
{
    return fwrite(ent, sizeof(struct bork_entity), 1, f);
}

static size_t write_ent_arr(FILE* f, bork_entity_arr_t* arr)
{
    uint32_t len = arr->len;
    size_t r = 0;
    r += sizeof(uint32_t) * fwrite(&len, sizeof(len), 1, f);
    r += sizeof(bork_entity_t) * fwrite(arr->data, sizeof(bork_entity_t), len, f);
    return r;
}

static size_t read_ent_arr(FILE* f, bork_entity_arr_t* arr)
{
    uint32_t len;
    size_t r = 0;
    r += sizeof(uint32_t) * fread(&len, sizeof(len), 1, f);
    printf("Reading ent array, length: %u\n", len);
    ARR_RESERVE_CLEAR(*arr, len);
    r += sizeof(bork_entity_t) * fread(arr->data, sizeof(bork_entity_t), len, f);
    arr->len = len;
    return r;
}

/*  Save file format:
            Player data
    Player entity               (1 struct bork_entity)
    Flashlight on               (1 int)
    Held datapads               (NUM_DATAPADS ints)
    Held schematics bitfield    (1 uint32_t)
    Installed upgrades          (3 ints)
    Upgrade levels              (3 ints)
    Upgrade counters            (3 ints)
    Selected upgrade level      (1 int)
    Selected upgrade            (1 int)
    Decoy active                (1 int)
    Decoy position              (1 vec3)
    Reload ticks                (1 int)
    Reload length               (1 int)
    Ammo counts                 (BORK_AMMO_TYPES ints)
    Held item id                (1 int)
    Quick items                 (4 ints)
    Inventory length            (1 int)
    Inventory ent id's          (^ ints)
    Held upgrades length        (1 int)
    Held upgrade id's           (^ ints)
            World data
    Current tick                (1 int)
    Global entity list length   (1 int)
    Global entity list          (^ struct bork_entity)
    Bullets length              (1 int)
    Bullets list                (^ struct bork_bullet)
    Map enemies length          (1 int)
    Map enemies list            (^ bork_entity_t)
    Map items length            (1 int)
    Map items list              (^ bork_entity_t)
    Map entities length         (1 int)
    Map entities list           (^ bork_entity_t)
    Map fires length            (1 int)
    Map fires list              (^ bork_fire)
    Map fire_obj length         (1 int)
    Map fire_obj list           (^ struct bork_map_object)
    Map doors length            (1 int)
    Map doors list              (^ struct bork_map_object)
    Map grates length           (1 int)
    Map grates list             (^ struct bork_map_object)
*/

void save_game(struct bork_play_data* d, char* name)
{
    char filename[1024];
    snprintf(filename, 1024, "%ssaves/%s", d->core->base_path, name);
    FILE* f = fopen(filename, "wb");
    if(!f) {
        printf("Could not open save file %s\n", filename);
        return;
    }
    size_t r = 0;
    /*  Player data */
    r += sizeof(struct bork_entity) * fwrite(&d->plr, sizeof(struct bork_entity), 1, f);
    r += sizeof(int) * fwrite(&d->flashlight_on, sizeof(int), 1, f);
    r += sizeof(int) * fwrite(d->held_datapads, sizeof(int), NUM_DATAPADS, f);
    r += sizeof(uint32_t) * fwrite(&d->held_schematics, sizeof(uint32_t), 1, f);
    r += sizeof(enum bork_upgrade) * fwrite(d->upgrades, sizeof(enum bork_upgrade), 3, f);
    r += sizeof(int) * fwrite(d->upgrade_level, sizeof(int), 3, f);
    r += sizeof(int) * fwrite(d->upgrade_counters, sizeof(int), 3, f);
    r += sizeof(int) * fwrite(&d->upgrade_use_level, sizeof(int), 1, f);
    r += sizeof(int) * fwrite(&d->upgrade_selected, sizeof(int), 1, f);
    r += sizeof(int) * fwrite(&d->decoy_active, sizeof(int), 1, f);
    r += sizeof(vec3) * fwrite(d->decoy_pos, sizeof(vec3), 1, f);
    r += sizeof(int) * fwrite(&d->reload_ticks, sizeof(int), 1, f);
    r += sizeof(int) * fwrite(&d->reload_length, sizeof(int), 1, f);
    r += sizeof(int) * fwrite(d->ammo, sizeof(int), BORK_AMMO_TYPES, f);
    r += sizeof(bork_entity_t) * fwrite(&d->held_item, sizeof(int), 1, f);
    r += sizeof(bork_entity_t) * fwrite(&d->quick_item, sizeof(int), 4, f);
    r += write_ent_arr(f, &d->inventory);
    r += write_ent_arr(f, &d->held_upgrades);
    /*  World data  */
    r += fwrite(&d->play_ticks, sizeof(int), 1, f);
    r += fwrite(&d->ticks, sizeof(int), 1, f);
    r += bork_entpool_write_to_file(f);
    uint32_t len = d->bullets.len;
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    r += sizeof(struct bork_bullet) * fwrite(d->bullets.data, sizeof(struct bork_bullet), len, f);
    int x,y,z;
    len = 0;
    for(x = 0; x < 4; ++x) for(y = 0; y < 4; ++y) for(z = 0; z < 4; ++z) {
        len += d->map.enemies[x][y][z].len;
    }
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    for(x = 0; x < 4; ++x) for(y = 0; y < 4; ++y) for(z = 0; z < 4; ++z) {
        if(d->map.enemies[x][y][z].len == 0) continue;
        r += sizeof(bork_entity_t) *
                fwrite(d->map.enemies[x][y][z].data, sizeof(bork_entity_t),
                       d->map.enemies[x][y][z].len, f);
    }
    len = 0;
    for(x = 0; x < 4; ++x) for(y = 0; y < 4; ++y) for(z = 0; z < 4; ++z) {
        len += d->map.items[x][y][z].len;
    }
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    for(x = 0; x < 4; ++x) for(y = 0; y < 4; ++y) for(z = 0; z < 4; ++z) {
        if(d->map.items[x][y][z].len == 0) continue;
        r += sizeof(bork_entity_t) *
                fwrite(d->map.items[x][y][z].data, sizeof(bork_entity_t),
                       d->map.items[x][y][z].len, f);
    }
    len = 0;
    for(x = 0; x < 4; ++x) for(y = 0; y < 4; ++y) for(z = 0; z < 4; ++z) {
        len += d->map.entities[x][y][z].len;
    }
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    for(x = 0; x < 4; ++x) for(y = 0; y < 4; ++y) for(z = 0; z < 4; ++z) {
        if(d->map.entities[x][y][z].len == 0) continue;
        r += sizeof(bork_entity_t) *
                fwrite(d->map.entities[x][y][z].data, sizeof(bork_entity_t),
                       d->map.entities[x][y][z].len, f);
    }
    /*  Fires   */
    len = d->map.fires.len;
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    r += sizeof(struct bork_fire) * fwrite(d->map.fires.data, sizeof(struct bork_fire), len, f);
    /*  Fire objects    */
    len = d->map.fire_objs.len;
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    r += sizeof(struct bork_map_object) * fwrite(d->map.fire_objs.data, sizeof(struct bork_map_object), len, f);
    /*  Doors   */
    len = d->map.doors.len;
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    r += sizeof(struct bork_map_object) * fwrite(d->map.doors.data, sizeof(struct bork_map_object), len, f);
    /*  Grates  */
    len = d->map.grates.len;
    r += sizeof(uint32_t) * fwrite(&len, sizeof(uint32_t), 1, f);
    r += sizeof(struct bork_map_object) * fwrite(d->map.grates.data, sizeof(struct bork_map_object), len, f);
    printf("Saved game: wrote %zu B\n", r);
    fclose(f);
}

void load_game(struct bork_play_data* d, char* name)
{
    char filename[1024];
    snprintf(filename, 1024, "%ssaves/%s", d->core->base_path, name);
    FILE* f = fopen(filename, "rb");
    if(!f) {
        printf("Could not open save file %s\n", filename);
        return;
    }
    bork_map_reset(&d->map);
    ARR_TRUNCATE_CLEAR(d->bullets, 0);
    ARR_TRUNCATE_CLEAR(d->particles, 0);
    size_t r = 0;
    /*  Player data */
    r += sizeof(struct bork_entity) * fread(&d->plr, sizeof(struct bork_entity), 1, f);
    r += sizeof(int) * fread(&d->flashlight_on, sizeof(int), 1, f);
    r += sizeof(int) * fread(d->held_datapads, sizeof(int), NUM_DATAPADS, f);
    r += sizeof(uint32_t) * fread(&d->held_schematics, sizeof(uint32_t), 1, f);
    r += sizeof(enum bork_upgrade) * fread(d->upgrades, sizeof(enum bork_upgrade), 3, f);
    r += sizeof(int) * fread(d->upgrade_level, sizeof(int), 3, f);
    r += sizeof(int) * fread(d->upgrade_counters, sizeof(int), 3, f);
    r += sizeof(int) * fread(&d->upgrade_use_level, sizeof(int), 1, f);
    r += sizeof(int) * fread(&d->upgrade_selected, sizeof(int), 1, f);
    r += sizeof(int) * fread(&d->decoy_active, sizeof(int), 1, f);
    r += sizeof(vec3) * fread(d->decoy_pos, sizeof(vec3), 1, f);
    r += sizeof(int) * fread(&d->reload_ticks, sizeof(int), 1, f);
    r += sizeof(int) * fread(&d->reload_length, sizeof(int), 1, f);
    r += sizeof(int) * fread(d->ammo, sizeof(int), BORK_AMMO_TYPES, f);
    r += sizeof(bork_entity_t) * fread(&d->held_item, sizeof(int), 1, f);
    r += sizeof(bork_entity_t) * fread(&d->quick_item, sizeof(int), 4, f);
    r += read_ent_arr(f, &d->inventory);
    r += read_ent_arr(f, &d->held_upgrades);
    /*  World data  */
    int tick;
    r += fread(&d->play_ticks, sizeof(int), 1, f);
    r += fread(&d->ticks, sizeof(int), 1, f);
    r += bork_entpool_read_from_file(f);
    uint32_t len = 0;
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    ARR_RESERVE_CLEAR(d->bullets, len);
    r += sizeof(struct bork_bullet) * fread(d->bullets.data, sizeof(struct bork_bullet), len, f);
    d->bullets.len = len;
    bork_entity_t ent_id;
    int i;
    len = 0;
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    for(i = 0; i < len; ++i) {
        r += sizeof(bork_entity_t) * fread(&ent_id, sizeof(bork_entity_t), 1, f);
        bork_map_add_enemy(&d->map, ent_id);
    }
    len = 0;
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    for(i = 0; i < len; ++i) {
        r += sizeof(bork_entity_t) * fread(&ent_id, sizeof(bork_entity_t), 1, f);
        bork_map_add_item(&d->map, ent_id);
    }
    len = 0;
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    for(i = 0; i < len; ++i) {
        r += sizeof(bork_entity_t) * fread(&ent_id, sizeof(bork_entity_t), 1, f);
        bork_map_add_entity(&d->map, ent_id);
    }
    /*  Fires   */
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    ARR_RESERVE_CLEAR(d->map.fires, len);
    r += sizeof(struct bork_fire) * fread(d->map.fires.data, sizeof(struct bork_fire), len, f);
    d->map.fires.len = len;
    /*  Fire objects    */
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    ARR_RESERVE_CLEAR(d->map.fire_objs, len);
    r += sizeof(struct bork_map_object) * fread(d->map.fire_objs.data, sizeof(struct bork_map_object), len, f);
    d->map.fire_objs.len = len;
    /*  Doors   */
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    ARR_RESERVE_CLEAR(d->map.doors, len);
    r += sizeof(struct bork_map_object) * fread(d->map.doors.data, sizeof(struct bork_map_object), len, f);
    d->map.doors.len = len;
    /*  Grates  */
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    ARR_RESERVE_CLEAR(d->map.grates, len);
    r += sizeof(struct bork_map_object) * fread(d->map.grates.data, sizeof(struct bork_map_object), len, f);
    d->map.grates.len = len;
    printf("Loaded game: read %zu B\n", r);
    d->map.plr = &d->plr;
    fclose(f);
    bork_play_reset_hud_anim(d);
}

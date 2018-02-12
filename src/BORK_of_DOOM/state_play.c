#include <stdlib.h>
#include <string.h>
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

static void bork_play_update(struct pg_game_state* state);
static void bork_play_tick(struct pg_game_state* state);
static void bork_play_draw(struct pg_game_state* state);

void bork_play_deinit(void* data)
{
    struct bork_play_data* d = data;
    if(d->fire_audio_handle >= 0) pg_audio_emitter_remove(d->fire_audio_handle);
    if(d->jp_audio_handle >= 0) pg_audio_emitter_remove(d->jp_audio_handle);
    if(d->music_audio_handle >= 0) pg_audio_emitter_remove(d->music_audio_handle);
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

void bork_play_reset(struct bork_play_data* d)
{
    d->scan_ticks = 0;
    d->hud_datapad_id = -1;
    d->hud_datapad_ticks = 0;
    d->hud_announce_ticks = 0;
    if(d->fire_audio_handle >= 0) pg_audio_emitter_remove(d->fire_audio_handle);
    if(d->jp_audio_handle >= 0) pg_audio_emitter_remove(d->jp_audio_handle);
    if(d->music_audio_handle >= 0) pg_audio_emitter_remove(d->music_audio_handle);
    ARR_TRUNCATE_CLEAR(d->particles, 0);
    ARR_TRUNCATE_CLEAR(d->plr_enemy_query, 0);
    ARR_TRUNCATE_CLEAR(d->plr_item_query, 0);
    ARR_TRUNCATE_CLEAR(d->plr_entity_query, 0);
    ARR_TRUNCATE_CLEAR(d->lights_buf, 0);
    ARR_TRUNCATE_CLEAR(d->spotlights, 0);
    ARR_TRUNCATE_CLEAR(d->inventory, 0);
    ARR_TRUNCATE_CLEAR(d->bullets, 0);
    bork_entpool_clear();
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
    /*  Crash   */
    //int* bill_gates_please_die = NULL;
    //bill_gates_please_die[4] = 1000;
    /*  After crash */
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
    d->fire_audio_handle = -1;
    d->jp_audio_handle = -1;
    d->music_audio_handle = -1;
    d->music_id = -1;
    d->draw_ui = 1;
    d->draw_weapon = 1;
    /*  Generate the BONZ station   */
    bork_map_init(&d->map);
    struct bork_editor_map ed_map = {};
    char filename[1024];
    snprintf(filename, 1024, "%sbork_map", core->base_path);
    bork_editor_load_map(&ed_map, filename);
    d->map.plr = &d->plr;
    bork_editor_complete_map(&d->map, &ed_map, 1);
    bork_map_init_model(&d->map, &ed_map, core);
    SDL_ShowCursor(SDL_DISABLE);
    pg_mouse_mode(0);
    d->menu.state = BORK_MENU_INTRO;
    d->menu.intro.slide = 0;
    d->menu.intro.ticks = PLAY_SECONDS(9);
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
        if(d->core->invert_y) mouse_motion[1] *= -1;
        vec2_scale(mouse_motion, mouse_motion, d->core->mouse_sensitivity);
        vec2_sub(d->mouse_motion, d->mouse_motion, mouse_motion);
        if(d->core->gpad_idx >= 0) {
            vec2 stick;
            pg_gamepad_stick(1, stick);
            vec2_scale(stick, stick, 0.05 * d->core->joy_sensitivity);
            vec2_sub(d->mouse_motion, d->mouse_motion, stick);
        }
    }
    if(pg_user_exit()) {
        state->running = 0;
    }
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
static void tick_music(struct bork_play_data* d);
static void tick_ambient_sound(struct bork_play_data* d);

void show_tut_message(struct bork_play_data* d, enum bork_play_tutorial_msg msg)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    if(d->tutorial_msg_seen & (1 << msg)) return;
    else {
        d->tutorial_msg_seen |= (1 << msg);
        char msg_text[64] = {};
        int gpad = (d->core->gpad_idx >= 0) ? 1 : 0;
        switch(msg) {
            case BORK_TUT_PICKUP_ITEM:
                snprintf(msg_text, 64, "<%s> PICK UP ITEMS",
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_INTERACT])
                              : pg_input_name(kmap[BORK_CTRL_INTERACT]));
                break;
            case BORK_TUT_VIEW_INVENTORY:
                snprintf(msg_text, 64, "<%s> OPEN MENU AND VIEW INVENTORY",
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_MENU])
                              : pg_input_name(kmap[BORK_CTRL_MENU]));
                break;
            case BORK_TUT_DATAPADS:
                snprintf(msg_text, 64, "DATAPADS CAN BE READ AGAIN IN THE MENU");
                break;
            case BORK_TUT_CROUCH_FLASHLIGHT:
                snprintf(msg_text, 64, "<%s> CROUCH    <%s> FLASHLIGHT",
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_CROUCH])
                              : pg_input_name(kmap[BORK_CTRL_CROUCH]),
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_FLASHLIGHT])
                              : pg_input_name(kmap[BORK_CTRL_FLASHLIGHT]));
                break;
            case BORK_TUT_SCHEMATIC:
                snprintf(msg_text, 64, "SCHEMATIC DETAILS AVAILABLE IN THE MENU");
                break;
            case BORK_TUT_RECYCLER:
                snprintf(msg_text, 64, "<%s> USE RECYCLER",
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_INTERACT])
                              : pg_input_name(kmap[BORK_CTRL_INTERACT]));
                break;
            case BORK_TUT_USE_UPGRADE:
                snprintf(msg_text, 64, "INSTALL UPGRADES IN THE MENU");
                break;
            case BORK_TUT_SWITCH_UPGRADE:
                snprintf(msg_text, 64, "<%s / %s> SWITCH / USE UPGRADES",
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_NEXT_TECH])
                              : pg_input_name(kmap[BORK_CTRL_NEXT_TECH]),
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_USE_TECH])
                              : pg_input_name(kmap[BORK_CTRL_USE_TECH]));
                break;
            case BORK_TUT_SWITCH_AMMO:
                snprintf(msg_text, 64, "SWITCH AMMUNITION IN YOUR INVENTORY");
                break;
            case BORK_TUT_DOORS:
                snprintf(msg_text, 64, "<%s> USE KEYPADS TO OPEN DOORS",
                         gpad ? pg_gamepad_name(gmap[BORK_CTRL_INTERACT])
                              : pg_input_name(kmap[BORK_CTRL_INTERACT]));
                break;
            default: snprintf(msg_text, 64, "TUTORIAL YEAH"); break;
        }
        hud_announce(d, msg_text);
    }
}

void bork_play_tick(struct pg_game_state* state)
{
    struct bork_play_data* d = state->data;
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    float ar = d->core->aspect_ratio;
    ++d->ticks;
    tick_music(d);
    /*  Handle input    */
    if(d->menu.state != BORK_MENU_CLOSED
    && !(d->menu.state == BORK_MENU_GAME && d->menu.game.mode == GAME_MENU_SHOW_OPTIONS)
    && d->menu.state < BORK_MENU_DOORPAD) {
        tick_menu_base(d);
    }
    if(d->menu.state == BORK_MENU_INVENTORY) tick_control_inv_menu(d);
    else if(d->menu.state == BORK_MENU_UPGRADES) tick_control_upgrade_menu(d);
    else if(d->menu.state == BORK_MENU_RECYCLER) tick_recycler_menu(d);
    else if(d->menu.state == BORK_MENU_DATAPADS) tick_datapad_menu(d);
    else if(d->menu.state == BORK_MENU_GAME) tick_game_menu(d, state);
    else if(d->menu.state == BORK_MENU_DOORPAD) tick_doorpad(d);
    else if(d->menu.state == BORK_MENU_INTRO) tick_intro(d);
    else if(d->menu.state == BORK_MENU_OUTRO) {
        if(d->menu.intro.slide == 9) {
            struct bork_game_core* core = d->core;
            bork_play_deinit(d);
            bork_menu_start(state, core);
            pg_mouse_mode(0);
            SDL_ShowCursor(SDL_ENABLE);
            pg_flush_input();
            return;
        } else tick_outro(d);
    } else if(d->menu.state == BORK_MENU_PLAYERDEAD) {
        int stick_ctrl_x = 0, stick_ctrl_y = 0;
        if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
            vec2 stick;
            pg_gamepad_stick(0, stick);
            if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
            else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
        }
        vec2 mouse_pos;
        int click;
        int clicked_option = 0;
        pg_mouse_pos(mouse_pos);
        vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0], 1 / d->core->screen_size[1] });
        click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
        if(fabs(mouse_pos[0] - (ar * 0.5)) < 0.5 && fabs(mouse_pos[1] - 0.595) < 0.03) {
            d->menu.gameover.opt_idx = 0;
            if(click) clicked_option = 1;
        } else if(fabs(mouse_pos[0] - (ar * 0.5)) < 0.5 && fabs(mouse_pos[1] - 0.695) < 0.03
               && d->core->save_files.len) {
            d->menu.gameover.opt_idx = 1;
            if(click) clicked_option = 1;
        }
        if((pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1)
        && d->core->save_files.len) {
            d->menu.gameover.opt_idx = 1;
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
            d->menu.gameover.opt_idx = 0;
        }
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || clicked_option) {
            if(d->menu.gameover.opt_idx == 0) {
                struct bork_game_core* core = d->core;
                bork_play_deinit(d);
                bork_menu_start(state, core);
                pg_mouse_mode(0);
                SDL_ShowCursor(SDL_ENABLE);
                pg_flush_input();
                return;
            } else {
                struct bork_save* save = &d->core->save_files.data[0];
                load_game(d, save->name);
                d->menu.game.mode = BORK_MENU_CLOSED;
                return;
            }
        }
    } else if(d->menu.state == BORK_MENU_CLOSED) {
        ++d->play_ticks;
        /*
        if(d->log_colls) {
            fprintf(d->logfile, "Tick %d, pos %f %f %f\n", d->play_ticks,
                    d->plr.pos[0], d->plr.pos[1], d->plr.pos[2]);
        }*/
        if(d->plr.pain_ticks > 0) --d->plr.pain_ticks;
        if(d->teleport_ticks > 0) --d->teleport_ticks;
        vec3 surr_start, surr_end;
        vec3_sub(surr_start, d->plr.pos, (vec3){ 32, 32, 8 });
        vec3_add(surr_end, d->plr.pos, (vec3){ 32, 32, 8 });
        ARR_TRUNCATE(d->plr_enemy_query, 0);
        ARR_TRUNCATE(d->plr_item_query, 0);
        ARR_TRUNCATE(d->plr_entity_query, 0);
        bork_map_query_enemies(&d->map, &d->plr_enemy_query, surr_start, surr_end);
        bork_map_query_items(&d->map, &d->plr_item_query, surr_start, surr_end);
        bork_map_query_entities(&d->map, &d->plr_entity_query, surr_start, surr_end);
        if(d->recoil_ticks > 0) {
            float r = (float)d->recoil_ticks / d->recoil_time;
            int strength = get_upgrade_level(d, BORK_UPGRADE_STRENGTH);
            if(strength == 0) r *= 0.5;
            else if(strength == 1) r *= 0.2;
            vec2 recoil;
            vec2_scale(recoil, d->recoil, r);
            vec2_add(d->plr.dir, d->plr.dir, recoil);
            --d->recoil_ticks;
        }
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
        if(d->hud_announce_ticks) --d->hud_announce_ticks;
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
    d->menu.datapads.viewing_dp = -1;
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
                pg_audio_play(&d->core->menu_sound, 0.5);
                d->menu.state = i + 1;
            }
        }
    }
    if(d->menu.game.mode != GAME_MENU_EDIT_SAVE
    && pg_check_input(SDL_SCANCODE_LSHIFT, PG_CONTROL_HELD)) {
        if(pg_check_input(SDL_SCANCODE_D, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            ++d->menu.state;
            if(d->menu.state > 5) d->menu.state = 1;
            pg_flush_input();
        } else if(pg_check_input(SDL_SCANCODE_A, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            --d->menu.state;
            if(d->menu.state == 0) d->menu.state = 5;
            pg_flush_input();
        }
    }
    if(pg_check_gamepad(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, PG_CONTROL_HIT)) {
        pg_audio_play(&d->core->menu_sound, 0.5);
        ++d->menu.state;
        if(d->menu.state > 5) d->menu.state = 1;
        pg_flush_input();
    } else if(pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        pg_audio_play(&d->core->menu_sound, 0.5);
        --d->menu.state;
        if(d->menu.state == 0) d->menu.state = 5;
        pg_flush_input();
    }
}

static void tick_music(struct bork_play_data* d)
{
    /*  Music logic */
    if(d->ticks % 10 == 0) {
        if(d->music_audio_handle != -1) {
            if(d->music_fade_dir) {
                float fade_vol = (float)d->music_fade_ticks / 360.0f;
                pg_audio_channel_volume(3, fade_vol * d->core->music_volume * 0.5);
                d->music_fade_ticks += d->music_fade_dir * 10;
                if(d->music_fade_ticks == 360) d->music_fade_dir = 0;
                if(d->music_fade_ticks <= 0 && d->music_fade_dir == -1) {
                    pg_audio_emitter_remove(d->music_audio_handle);
                    d->music_audio_handle = -1;
                }
            }
        }
        if(d->music_id != -1 && d->music_fade_ticks == 0
        && d->music_audio_handle == -1) {
            pg_audio_channel_volume(3, 0);
            d->music_audio_handle =
                pg_audio_emitter(&d->core->sounds[d->music_id], 1, 1, (vec3){}, 3);
            d->music_fade_dir = 1;
        }
        if(d->plr.pos[2] > 52) {
            if(d->plr.pos[1] > 52) {
                if(d->music_id != BORK_MUS_ENDGAME) {
                    d->music_id = BORK_MUS_ENDGAME;
                    if(d->music_fade_ticks) d->music_fade_dir = -1;
                    else d->music_fade_dir = 1;
                }
            } else {
                if(d->music_id != BORK_MUS_BOSSFIGHT) {
                    d->music_id = BORK_MUS_BOSSFIGHT;
                    if(d->music_fade_ticks) d->music_fade_dir = -1;
                    else d->music_fade_dir = 1;
                }
            }
        } else if(d->music_id != -1) {
            d->music_id = -1;
            d->music_fade_dir = -1;
        }
    }
}

static void tick_ambient_sound(struct bork_play_data* d)
{
    vec3 plr_pos;
    vec3_mul(plr_pos, d->plr.pos, (vec3){ 1, 1, 2 });
    pg_audio_set_listener(1, plr_pos);
    int i;
    struct bork_sound_emitter* snd;
    ARR_FOREACH_PTR(d->map.sounds, snd, i) {
        if((i + d->play_ticks) % 20 != 0) continue;
        vec3 snd_pos;
        vec3_mul(snd_pos, snd->pos, (vec3){ 1, 1, 2 });
        float dist = vec3_dist2(snd_pos, plr_pos);
        if(dist < (snd->area * snd->area)) {
            if(snd->snd == BORK_SND_HISS) {
                if(d->play_ticks >= snd->next_play) {
                    snd->next_play = d->play_ticks + (RANDF * PLAY_SECONDS(5) + PLAY_SECONDS(5));
                    pg_audio_emit_once(&d->core->sounds[snd->snd], snd->volume, snd->area, snd_pos, 1);
                }
                continue;
            }
            if(snd->handle >= 0) continue;
            else snd->handle = pg_audio_emitter(&d->core->sounds[snd->snd], snd->volume,
                                                snd->area, snd_pos, 1);
        } else if(snd->handle >= 0) {
            pg_audio_emitter_remove(snd->handle);
            snd->handle = -1;
        }
    }
    if((d->plr.flags & BORK_ENTFLAG_GROUND) && vec2_len(d->plr.vel) > 0.01) {
        if(d->walk_input_ticks % 90 == 10) {
            pg_audio_play(&d->core->sounds[BORK_SND_FOOTSTEP1], 0.25);
        } else if(d->walk_input_ticks % 90 == 55) {
            pg_audio_play(&d->core->sounds[BORK_SND_FOOTSTEP2], 0.25);
        }
    }
}

static void tick_control_play(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    /*
    if(pg_check_input(SDL_SCANCODE_M, PG_CONTROL_HIT)) {
        if(pg_check_input(SDL_SCANCODE_LSHIFT, PG_CONTROL_HELD)) {
            pg_mouse_mode(0);
            SDL_ShowCursor(SDL_ENABLE);
        } else {
            pg_mouse_mode(1);
            SDL_ShowCursor(SDL_DISABLE);
        }
    }
    if(pg_check_input(SDL_SCANCODE_F10, PG_CONTROL_HIT)) {
        d->log_colls = !d->log_colls;
        if(d->log_colls) hud_announce(d, "COLLISION LOGGING: ON");
        else hud_announce(d, "COLLISION LOGGING: OFF");
    }
    if(pg_check_input(SDL_SCANCODE_GRAVE, PG_CONTROL_HIT)) {
        vec3 push = { RANDF - 0.5, RANDF - 0.5, RANDF * -0.5 };
        vec3_scale(push, push, 0.1);
        //vec3_add(d->plr.vel, d->plr.vel, push);
        d->plr.vel[2] += push[2];
    }*/
    if(d->plr.HP <= 0) return;
    if(pg_check_input(SDL_SCANCODE_F12, PG_CONTROL_HIT)) {
        d->draw_ui = !d->draw_ui;
    }
    if(pg_check_input(SDL_SCANCODE_F11, PG_CONTROL_HIT)) {
        d->draw_weapon = !d->draw_weapon;
    }
    if(pg_check_input(kmap[BORK_CTRL_MENU], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_MENU], PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_INVENTORY;
        pg_audio_channel_pause(1, 1);
        pg_audio_channel_pause(2, 1);
        reset_menus(d);
        SDL_ShowCursor(SDL_ENABLE);
        pg_mouse_mode(0);
    }
    if(pg_check_input(kmap[BORK_CTRL_NEXT_TECH], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_NEXT_TECH], PG_CONTROL_HIT)) {
        select_next_upgrade(d, 1);
    } else if(pg_check_input(kmap[BORK_CTRL_PREV_TECH], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_PREV_TECH], PG_CONTROL_HIT)) {
        select_next_upgrade(d, -1);
    }
    if(pg_check_input(kmap[BORK_CTRL_NEXT_ITEM], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_NEXT_ITEM], PG_CONTROL_HIT)) {
        next_item(d, 1);
    } else if(pg_check_input(kmap[BORK_CTRL_PREV_ITEM], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_PREV_ITEM], PG_CONTROL_HIT)) {
        next_item(d, -1);
    }
    if(pg_check_input(kmap[BORK_CTRL_DROP], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_DROP], PG_CONTROL_HIT)) {
        drop_item(d);
    }
    /*  Check if we're looking at an item and need to show the tutorial */
    if(d->looked_item != -1) {
        struct bork_entity* item = bork_entity_get(d->looked_item);
        if(vec3_dist2(item->pos, d->plr.pos) < (3 * 3)) show_tut_message(d, BORK_TUT_PICKUP_ITEM);
    } else if(d->looked_obj) {
        if(d->looked_obj->type == BORK_MAP_DOORPAD) show_tut_message(d, BORK_TUT_DOORS);
        else if(d->looked_obj->type == BORK_MAP_RECYCLER) show_tut_message(d, BORK_TUT_RECYCLER);
    }
    if(pg_check_input(kmap[BORK_CTRL_INTERACT], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_INTERACT], PG_CONTROL_HIT)) {
        struct bork_entity* item = bork_entity_get(d->looked_item);
        vec3 eye_pos, eye_dir;
        bork_entity_get_eye(&d->plr, eye_dir, eye_pos);
        eye_pos[2] -= 0.2;
        if(item && vec3_dist(item->pos, eye_pos) <= 2.5
        && bork_map_check_vis(&d->map, eye_pos, item->pos)) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
            if(item->flags & BORK_ENTFLAG_IS_AMMO) {
                int ammo_type = item->type - BORK_ITEM_BULLETS;
                if(item->type != BORK_ITEM_BULLETS
                && item->type != BORK_ITEM_SHELLS
                && item->type != BORK_ITEM_PLAZMA)
                    show_tut_message(d, BORK_TUT_SWITCH_AMMO);
                d->ammo[ammo_type] += prof->ammo_capacity;
                item->flags |= BORK_ENTFLAG_DEAD;
            } else if(item->type == BORK_ITEM_DATAPAD) {
                show_tut_message(d, BORK_TUT_DATAPADS);
                d->hud_datapad_id = item->counter[0];
                d->hud_datapad_ticks = -500;
                d->held_datapads[d->num_held_datapads++] = item->counter[0];
                item->flags |= BORK_ENTFLAG_DEAD;
            } else if(item->type == BORK_ITEM_UPGRADE) {
                show_tut_message(d, BORK_TUT_USE_UPGRADE);
                item->flags |= BORK_ENTFLAG_IN_INVENTORY;
                ARR_PUSH(d->held_upgrades, d->looked_item);
            } else if(item->type == BORK_ITEM_SCHEMATIC) {
                show_tut_message(d, BORK_TUT_SCHEMATIC);
                item->flags |= BORK_ENTFLAG_DEAD;
                d->held_schematics |= (1 << item->counter[0]);
            } else {
                show_tut_message(d, BORK_TUT_VIEW_INVENTORY);
                item->flags |= BORK_ENTFLAG_IN_INVENTORY;
                add_inventory_item(d, d->looked_item);
            }
            pg_audio_play(&d->core->sounds[BORK_SND_PICKUP], 0.5);
        } else if(d->looked_obj) {
            if(d->looked_obj->type == BORK_MAP_DOORPAD) {
                vec3 eye_to_pad;
                vec3_sub(eye_to_pad, d->looked_obj->pos, eye_pos);
                float dist = vec3_dist(eye_pos, d->looked_obj->pos);
                float vis_dist = bork_map_vis_dist(&d->map, eye_pos, eye_to_pad, 2);
                if(dist - vis_dist < 0.75) {
                    struct bork_map_object* door = &d->map.doors.data[d->looked_obj->doorpad.door_idx];
                    if(door->door.locked && d->looked_obj->doorpad.locked_side) {
                        d->menu.doorpad.unlocked_ticks = 0;
                        d->menu.doorpad.selection[0] = -1;
                        d->menu.doorpad.selection[1] = 0;
                        d->menu.doorpad.num_chars = 0;
                        d->menu.doorpad.door_idx = d->looked_obj->doorpad.door_idx;
                        d->menu.state = BORK_MENU_DOORPAD;
                        SDL_ShowCursor(SDL_ENABLE);
                        pg_mouse_mode(0);
                    } else {
                        if(!door->door.open) {
                            pg_audio_play_ch(&d->core->sounds[BORK_SND_DOOR_OPEN], 0.5, 1);
                            door->door.locked = 0;
                            door->door.open = 1;
                        } else {
                            if(vec2_dist2(door->pos, d->plr.pos) < (0.7 * 0.7)) {
                                pg_audio_play_ch(&d->core->sounds[BORK_SND_BUZZ], 2, 1);
                            } else {
                                pg_audio_play_ch(&d->core->sounds[BORK_SND_DOOR_CLOSE], 0.5, 1);
                                door->door.open = 0;
                            }
                        }
                    }
                }
            } else if(d->looked_obj->type == BORK_MAP_RECYCLER) {
                reset_menus(d);
                d->menu.state = BORK_MENU_RECYCLER;
                d->menu.recycler.obj = d->looked_obj;
                SDL_ShowCursor(SDL_ENABLE);
                pg_mouse_mode(0);
                pg_audio_channel_pause(1, 1);
                pg_audio_channel_pause(2, 1);
            } else if(d->looked_obj->type == BORK_MAP_ESCAPE_POD) {
                pg_mouse_mode(0);
                d->menu.state = BORK_MENU_OUTRO;
                d->menu.intro.ticks = 0;
                d->menu.intro.slide = 0;
            }
        }
    }
    if(d->held_item >= 0 && !d->reload_ticks && !d->switch_ticks
    && (pg_check_input(kmap[BORK_CTRL_RELOAD], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_RELOAD], PG_CONTROL_HIT))) {
        struct bork_entity* ent = bork_entity_get(d->inventory.data[d->held_item]);
        const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
        if(ent->flags & BORK_ENTFLAG_IS_GUN) {
            int ammo_type = prof->use_ammo[ent->ammo_type] - BORK_ITEM_BULLETS;
            int ammo_missing = prof->ammo_capacity - ent->ammo;
            int ammo_held = MIN(d->ammo[ammo_type], ammo_missing);
            if(ammo_held > 0) {
                pg_audio_play_ch(&d->core->sounds[BORK_SND_RELOAD_START], 1, 2);
                d->reload_ticks = prof->reload_time;
                d->reload_length = prof->reload_time;
            }
        }
    }
    if((pg_check_input(kmap[BORK_CTRL_JUMP], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_JUMP], PG_CONTROL_HIT))
    && d->plr.flags & BORK_ENTFLAG_GROUND) {
        if(d->plr.flags & BORK_ENTFLAG_CROUCH) d->plr.vel[2] = 0.11;
        else d->plr.vel[2] = 0.15;
        d->plr.flags &= ~BORK_ENTFLAG_GROUND;
        d->jump_released = 0;
        pg_audio_play(&d->core->sounds[BORK_SND_JUMP], 0.1);
    }
    if(pg_check_input(kmap[BORK_CTRL_JUMP], PG_CONTROL_RELEASED)
    || pg_check_gamepad(gmap[BORK_CTRL_JUMP], PG_CONTROL_RELEASED)) {
        d->jump_released = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_CROUCH], PG_CONTROL_HELD)
    || pg_check_gamepad(gmap[BORK_CTRL_CROUCH], PG_CONTROL_HELD)) {
        d->plr.flags |= BORK_ENTFLAG_CROUCH;
    } else if(d->plr.flags & BORK_ENTFLAG_CROUCH) {
        vec3 check_pos;
        vec3_sub(check_pos, d->plr.pos, (vec3){ 0, 0, 0.1 });
        if(bork_map_vis_dist(&d->map, check_pos, (vec3){ 0, 0, 1 }, 1.5) >= 1) {
            //fprintf(d->logfile, "Can uncrouch\n");
            d->plr.flags &= ~BORK_ENTFLAG_CROUCH;
        }
    }
    int kbd_input = 0;
    float move_speed = d->player_speed * (d->plr.flags & BORK_ENTFLAG_GROUND ? 1 : 0.125);
    if(d->plr.flags & BORK_ENTFLAG_CROUCH) move_speed *= 0.6;
    vec2 move_dir = {};
    d->plr.flags &= ~BORK_ENTFLAG_SLIDE;
    if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        move_dir[0] -= sin(d->plr.dir[0]);
        move_dir[1] += cos(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        move_dir[0] += sin(d->plr.dir[0]);
        move_dir[1] -= cos(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        move_dir[0] += cos(d->plr.dir[0]);
        move_dir[1] += sin(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT | PG_CONTROL_HELD)) {
        move_dir[0] -= cos(d->plr.dir[0]);
        move_dir[1] -= sin(d->plr.dir[0]);
        kbd_input = 1;
    }
    if(kbd_input) {
        vec2_set_len(move_dir, move_dir, move_speed);
        vec2_add(d->plr.vel, d->plr.vel, move_dir);
    }
    if(vec2_len(d->plr.vel) > 0.1) vec2_set_len(d->plr.vel, d->plr.vel, 0.1);
    if(!kbd_input) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(stick[0] != 0 || stick[1] != 0) {
            kbd_input = 1;
            float c = cos(-d->plr.dir[0]), s = sin(d->plr.dir[0]);
            vec2 stick_rot = {
                stick[0] * s - stick[1] * c,
                stick[0] * c + stick[1] * s };
            float len = vec2_len(stick_rot);
            if(len > 0.0001) {
                if(len > 1) vec2_set_len(stick_rot, stick_rot, 1);
                d->plr.vel[0] += move_speed * stick_rot[0];
                d->plr.vel[1] -= move_speed * stick_rot[1];
                kbd_input = 1;
            }
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
    || pg_check_gamepad(gmap[BORK_CTRL_FLASHLIGHT], PG_CONTROL_HIT)) {
        d->flashlight_on = 1 - d->flashlight_on;
    }
    if(d->held_item != -1) {
        bork_entity_t held_id = d->inventory.data[d->held_item];
        struct bork_entity* held_ent = bork_entity_get(held_id);
        if(held_ent) {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[held_ent->type];
            if(!d->reload_ticks && !d->switch_ticks && prof->use_ctrl
            && (pg_check_input(kmap[BORK_CTRL_FIRE], prof->use_ctrl)
            || pg_check_gamepad(gmap[BORK_CTRL_FIRE], prof->use_ctrl))) {
                prof->use_func(held_ent, d);
            }
        }
    }
    if(pg_check_input(kmap[BORK_CTRL_BIND1], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_BIND1], PG_CONTROL_HIT)) {
        switch_item(d, 0);
    } else if(pg_check_input(kmap[BORK_CTRL_BIND2], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_BIND2], PG_CONTROL_HIT)) {
        switch_item(d, 1);
    } else if(pg_check_input(kmap[BORK_CTRL_BIND3], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_BIND3], PG_CONTROL_HIT)) {
        switch_item(d, 2);
    } else if(pg_check_input(kmap[BORK_CTRL_BIND4], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_BIND4], PG_CONTROL_HIT)) {
        switch_item(d, 3);
    }
    if(d->using_jetpack) {
        if(d->play_ticks % 10 == 0) {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT,
                .pos = { d->plr.pos[0] + (RANDF * 0.25 - 0.125),
                         d->plr.pos[1] + (RANDF * 0.25 - 0.125),
                         d->plr.pos[2] - 0.4 },
                .vel = { RANDF * 0.08 - 0.04, RANDF * 0.08 - 0.04, RANDF * -0.1 },
                .ticks_left = 240 + (RANDF * 20),
                .frame_ticks = 0,
                .current_frame = 24 + rand() % 4,
            };
            ARR_PUSH(d->particles, new_part);
        }
        if(d->jp_audio_handle == -1) {
            d->jp_audio_handle =
                pg_audio_emitter(&d->core->sounds[BORK_SND_FIRE], 0.5, 1, (vec3){}, 2);
        }
    } else if(d->jp_audio_handle >= -1) {
        pg_audio_emitter_remove(d->jp_audio_handle);
        d->jp_audio_handle = -1;
    }
}

static void tick_player(struct bork_play_data* d)
{
    //d->plr.HP = 100;
    /*  Check for auto-reload   */

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
            surr_ent->still_ticks = 0;
            surr_ent->flags &= ~BORK_ENTFLAG_INACTIVE;
        }
    }
    if(d->plr.flags & BORK_ENTFLAG_ON_FIRE) {
        entity_on_fire(d, &d->plr);
        if(d->plr.fire_ticks && d->fire_audio_handle == -1) {
            d->fire_audio_handle =
                pg_audio_emitter(&d->core->sounds[BORK_SND_FIRE], 0.5, 1, (vec3){}, 2);
        } else if(!d->plr.fire_ticks && d->fire_audio_handle >= 0) {
            pg_audio_emitter_remove(d->fire_audio_handle);
            d->fire_audio_handle = -1;
        }
        if(d->play_ticks % 10 == 0) {
            int fire_ticks = d->plr.fire_ticks;
            float vol = 2.5;
            if(fire_ticks < 120) vol *= (float)fire_ticks / 120;
            float loop_time = 10.0f / 120.0f;
            float loop_origin = (float)d->play_ticks / 120.0f;
        }
    }
    bork_entity_update(&d->plr, d);
    if(d->plr.dead_ticks >= 120) {
        d->menu.state = BORK_MENU_PLAYERDEAD;
        pg_mouse_mode(0);
        SDL_ShowCursor(SDL_ENABLE);
    } else if(d->plr.HP <= 0) ++d->plr.dead_ticks;
    else {
        vec2_set(d->plr.dir, d->plr.dir[0] + d->mouse_motion[0],
                             d->plr.dir[1] + d->mouse_motion[1]);
        vec2_set(d->mouse_motion, 0, 0);
        d->plr.dir[0] = fmodf(d->plr.dir[0], M_PI * 2.0f);
        d->plr.dir[1] = CLAMP(d->plr.dir[1], -M_PI * 0.5, M_PI * 0.5);
    }
}

static void tick_held_item(struct bork_play_data* d)
{
    if(d->held_item < 0) return;
    struct bork_entity* item = bork_entity_get(d->inventory.data[d->held_item]);
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[item->type];
    if(d->reload_ticks) {
        --d->reload_ticks;
        if(d->reload_ticks == 60) {
            pg_audio_play_ch(&d->core->sounds[BORK_SND_RELOAD_END], 1, 2);
            int ammo_type = prof->use_ammo[item->ammo_type] - BORK_ITEM_BULLETS;
            int ammo_missing = prof->ammo_capacity - item->ammo;
            int ammo_held = MIN(d->ammo[ammo_type], ammo_missing);
            if(ammo_held > 0) {
                d->ammo[ammo_type] -= ammo_held;
                item->ammo += ammo_held;
            }
        }
        return;
    } else if(d->switch_ticks) {
        --d->switch_ticks;
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
    } else if(!d->hud_anim_active && item->flags & BORK_ENTFLAG_IS_GUN && item->ammo == 0) {
        int ammo_type = prof->use_ammo[item->ammo_type] - BORK_ITEM_BULLETS;
        int ammo_held = d->ammo[ammo_type];
        if(ammo_held > 0) {
            pg_audio_play_ch(&d->core->sounds[BORK_SND_RELOAD_START], 1, 2);
            d->reload_ticks = prof->reload_time;
            d->reload_length = prof->reload_time;
        }
    }
}

static void tick_datapad(struct bork_play_data* d)
{
    if(d->hud_datapad_id < 0) return;
    const struct bork_datapad* dp = &BORK_DATAPADS[d->hud_datapad_id];
    int scan_level = get_upgrade_level(d, BORK_UPGRADE_SCANNING);
    int translate = 0;
    if(dp->lines_translated && scan_level == 1) translate = 1;
    ++d->hud_datapad_ticks;
    float text_line = (float)d->hud_datapad_ticks / 190.0f;
    int lines = translate ? dp->lines_translated : dp->lines;
    if(text_line > lines) {
        d->hud_datapad_ticks = 0;
        d->hud_datapad_id = -1;
    }
}

static void tick_enemies(struct bork_play_data* d)
{
    vec3 plr_pos;
    vec3_mul(plr_pos, d->plr.pos, (vec3){ 1, 1, 3 });
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
            if(ent->last_tick == d->play_ticks) continue;
            vec3 ent_pos;
            vec3_mul(ent_pos, ent->pos, (vec3){ 1, 1, 3 });
            if(vec3_dist2(ent_pos, plr_pos) > (48 * 48)) continue;
            if(!ent->aware_ticks && ((d->play_ticks + ent_id) % 20)) continue;
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
            if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                ent->last_tick = d->play_ticks;
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
            if((ent_id + d->play_ticks) % 30 == 0) {
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
    vec3 plr_pos;
    vec3_mul(plr_pos, d->plr.pos, (vec3){ 1, 1, 4 });
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
            vec3 ent_pos;
            vec3_mul(ent_pos, ent->pos, (vec3){ 1, 1, 4 });
            if(vec3_dist2(ent_pos, plr_pos) > (32 * 32)) continue;
            if(ent->last_tick == d->play_ticks) continue;
            else if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
                ent->last_tick = d->play_ticks;
                bork_entity_update(ent, d);
                if(((d->play_ticks + ent_id) & 16) == 0) {
                    ARR_TRUNCATE(surr, 0);
                    vec3 start, end;
                    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
                    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
                    bork_map_query_entities(&d->map, &surr, start, end);
                    /*  Physics against other enemies   */
                    int j;
                    struct bork_entity* surr_ent;
                    bork_entity_t surr_id;
                    ARR_FOREACH(surr, surr_id, j) {
                        if(surr_id == ent_id) continue;
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
                            ent->still_ticks = 0;
                            ent->flags &= ~BORK_ENTFLAG_INACTIVE;
                        }
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
            if(vec3_dist2(ent->pos, d->plr.pos) > (32 * 32)) continue;
            if(ent->last_tick == d->play_ticks) continue;
            else if(ent->pos[0] >= (x * 16.0f) && ent->pos[0] <= ((x + 1) * 16.0f)
            && ent->pos[1] >= (y * 16.0f) && ent->pos[1] <= ((y + 1) * 16.0f)
            && ent->pos[2] >= (z * 16.0f) && ent->pos[2] <= ((z + 1) * 16.0f)) {
                ent->last_tick = d->play_ticks;
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
        if(vec3_dist2(part->pos, d->plr.pos) > (32 * 32)) {
            ARR_SWAPSPLICE(d->particles, i, 1);
            continue;
        }
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
            vec3 audio_pos;
            vec3_mul(audio_pos, fire->pos, (vec3){ 1, 1, 2 });
            fire->audio_handle =
                pg_audio_emitter(&d->core->sounds[BORK_SND_FIRE], 1, 6, audio_pos, 1);
        }
        if(fire->flags & BORK_FIRE_MOVES) {
            vec3_add(fire->pos, fire->pos, fire->vel);
            fire->vel[2] -= 0.005;
            struct bork_collision coll;
            int hit = bork_map_collide(&d->map, &coll, fire->pos, (vec3){ 0.3, 0.3, 0.3 });
            if(hit) {
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
                if(!surr_ent || (surr_ent->flags & BORK_ENTFLAG_FIREPROOF)
                || surr_ent->last_fire_tick == d->play_ticks) continue;
                surr_ent->last_fire_tick = d->play_ticks;
                surr_ent->HP -= 5;
                if(rand() % 3 == 0) {
                    surr_ent->flags |= BORK_ENTFLAG_ON_FIRE;
                    surr_ent->fire_ticks = PLAY_SECONDS(2.5);
                }
            }
            if(plr_heatshield_lvl < 1 && d->plr.last_fire_tick != d->play_ticks
            && vec2_dist(fire->pos, d->plr.pos) < 1.5
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
        if(fire->lifetime <= 0) {
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
    vec3 vel_lerp = {}, draw_pos;
    vec2 draw_dir;
    vec3_scale(vel_lerp, d->plr.vel, t);
    vec3_add(draw_pos, d->plr.pos, vel_lerp);
    vec3 draw_coll_size;
    vec3_dup(draw_coll_size, BORK_ENT_PROFILES[BORK_ENTITY_PLAYER].size);
    if(d->plr.HP <= 0) draw_coll_size[2] *= 0.2;
    else if(d->plr.flags & BORK_ENTFLAG_CROUCH) draw_coll_size[2] *= 0.5;
    struct bork_collision coll = {};
    bork_map_collide(&d->map, &coll, draw_pos, draw_coll_size);
    if(coll.push[0] != 0 || coll.push[1] != 0 || coll.push[2] != 0) {
        float face_down_angle = vec3_mul_inner(coll.face_norm, PG_DIR_VEC[PG_UP]);
        vec3 ell_norm = {
            coll.push[0] / (draw_coll_size[0] * draw_coll_size[0]),
            coll.push[1] / (draw_coll_size[1] * draw_coll_size[1]),
            coll.push[2] / (draw_coll_size[2] * draw_coll_size[2]) };
        vec3_normalize(ell_norm, ell_norm);
        float down_angle = vec3_mul_inner(ell_norm, coll.face_norm);
        if(down_angle >= 0.9 && face_down_angle > 0.75 && face_down_angle < 0.9) {
            //vec3_set(coll.push, 0, 0, coll.push[2]);
            vec3_sub(draw_pos, draw_pos, (vec3){ 0, 0, vel_lerp[2] });
        } else vec3_add(draw_pos, draw_pos, coll.push);
    }
    vec3_add(draw_pos, draw_pos, (vec3){ 0, 0, draw_coll_size[2] * 0.9 });
    if(d->plr.HP > 0) {
        if(d->core->invert_y) vec2_sub(draw_dir, d->plr.dir, d->mouse_motion);
        else vec2_add(draw_dir, d->plr.dir, d->mouse_motion);
    } else vec2_dup(draw_dir, d->plr.dir);
    pg_viewer_set(&d->core->view, draw_pos, draw_dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->core->gbuf);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    if(d->draw_weapon) {
        pg_shader_3d_texture(&d->core->shader_3d, &d->core->item_tex);
        pg_shader_begin(&d->core->shader_3d, &d->core->view);
        draw_weapon(d, d->hud_anim_progress + (d->hud_anim_speed * t),
                    draw_pos, draw_dir);
    }
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
                           look_dir, 0.45);
        ARR_PUSH(d->spotlights, flashlight);
    }
    if(d->using_jetpack) {
        int jp_idx = 0;
        if(d->upgrades[0] == BORK_UPGRADE_JETPACK) jp_idx = 0;
        else if(d->upgrades[1] == BORK_UPGRADE_JETPACK) jp_idx = 1;
        else if(d->upgrades[2] == BORK_UPGRADE_JETPACK) jp_idx = 2;
        struct pg_light jp_light;
        vec3 light_pos;
        vec3_dup(light_pos, draw_pos);
        light_pos[2] -= 1.5;
        float strength = (float)d->upgrade_counters[jp_idx] / PLAY_SECONDS(10) + 0.1;
        strength = MIN(0.3, strength) * 10;
        pg_light_pointlight(&jp_light, light_pos, strength, (vec3){ 1.5, 1.5, 1 });
        ARR_PUSH(d->lights_buf, jp_light);
    }
    draw_lights(d);
    pg_ppbuffer_swapdst(&d->core->ppbuf);
    pg_gbuffer_finish(&d->core->gbuf, (vec3){ 0.025, 0.025, 0.025 });
    if(d->menu.state == BORK_MENU_CLOSED) {
        //pg_ppbuffer_swapdst(&d->core->ppbuf);
        if(d->draw_ui) {
            draw_hud_overlay(d);
            draw_datapad(d);
        }
    } else {
        /*  Finish to the post-process buffer so we can do the blur effect  */
        if(d->menu.state != BORK_MENU_INTRO && d->menu.state != BORK_MENU_OUTRO) {
            pg_postproc_blur_scale(&d->core->post_blur, (vec2){ 3, 3 });
            pg_postproc_blur_dir(&d->core->post_blur, 1);
            pg_ppbuffer_swapdst(&d->core->ppbuf);
            pg_postproc_apply(&d->core->post_blur, &d->core->ppbuf);
            pg_postproc_blur_dir(&d->core->post_blur, 0);
            pg_ppbuffer_swapdst(&d->core->ppbuf);
            pg_postproc_apply(&d->core->post_blur, &d->core->ppbuf);
            bork_draw_backdrop(d->core, (vec4){ 2, 2, 2, 0.5 },
                               (float)state->ticks / (float)state->tick_rate);
            bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 0.9 });
        }
        pg_shader_begin(&d->core->shader_2d, NULL);
        if(d->menu.state < BORK_MENU_DOORPAD
        && !(d->menu.state == BORK_MENU_GAME
          && d->menu.game.mode == GAME_MENU_SHOW_OPTIONS)) draw_active_menu(d);
        switch(d->menu.state) {
        default: break;
        case BORK_MENU_OUTRO:
            draw_outro(d, (float)state->ticks / (float)state->tick_rate);
            break;
        case BORK_MENU_INTRO:
            draw_intro(d, (float)state->ticks / (float)state->tick_rate);
            break;
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
    pg_ppbuffer_swapdst(&d->core->ppbuf);
    pg_screen_dst();
    pg_postproc_apply(&d->core->post_gamma, &d->core->ppbuf);
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
    if(d->core->gpad_idx != -1) {
        pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.75 }, (vec4){});
        pg_shader_2d_tex_frame(shader, 246);
        pg_shader_2d_transform(shader, (vec2){ ar * 0.75 + (-2 * ar * 0.08) - 0.025, 0.04 },
                               (vec2){ 0.025, 0.025 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_tex_frame(shader, 247);
        pg_shader_2d_transform(shader, (vec2){ ar * 0.75 + (2 * ar * 0.08) + 0.025, 0.04 },
                                       (vec2){ 0.025, 0.025 }, 0);
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
    int looking_at_upgrade = 0;
    enum bork_upgrade looked_upgrade[2] = { -1, -1 };
    float name_center = 0;
    struct bork_entity* looked_ent;
    int ti = 0;
    int len;
    struct pg_shader_text text = { .use_blocks = 0 };
    vec3 eye_pos, eye_dir;
    bork_entity_get_eye(&d->plr, eye_dir, eye_pos);
    eye_pos[2] -= 0.2;
    if(d->looked_item != -1 && (looked_ent = bork_entity_get(d->looked_item))
    && vec3_dist2(looked_ent->pos, eye_pos) < (2.5 * 2.5)) {
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
        } else if(looked_ent->type == BORK_ITEM_UPGRADE) {
            looking_at_upgrade = 1;
            looked_upgrade[0] = looked_ent->counter[0];
            looked_upgrade[1] = looked_ent->counter[1];
        }
    }
    if(d->scan_ticks) {
        len = snprintf(text.block[++ti], 64, "%s", d->scanned_name);
        vec4_set(text.block_style[ti], 0.2 - (len * 0.02 * 1.25 * 0.5), 0.6, 0.02, 1.25);
        float scan_alpha;
        if(d->scan_ticks > PLAY_SECONDS(2.75)) {
            scan_alpha = PLAY_SECONDS(3) - d->scan_ticks;
            scan_alpha /= PLAY_SECONDS(0.25);
        } else if(d->scan_ticks < PLAY_SECONDS(0.25)) {
            scan_alpha = d->scan_ticks;
            scan_alpha /= PLAY_SECONDS(0.25);
        } else scan_alpha = 1;
        vec4_set(text.block_color[ti], 1, 1, 1, scan_alpha);
    }
    if(d->hud_announce_ticks) {
        len = snprintf(text.block[++ti], 64, "%s", d->hud_announce);
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.02 * 1.25 * 0.5), 0.2, 0.02, 1.25);
        float announce_alpha;
        if(d->hud_announce_ticks > PLAY_SECONDS(3.25)) {
            announce_alpha = PLAY_SECONDS(3.5) - d->hud_announce_ticks;
            announce_alpha /= PLAY_SECONDS(0.25);
        } else if(d->hud_announce_ticks < PLAY_SECONDS(0.25)) {
            announce_alpha = d->hud_announce_ticks;
            announce_alpha /= PLAY_SECONDS(0.25);
        } else announce_alpha = 1;
        vec4_set(text.block_color[ti], 1, 1, 1, announce_alpha);
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
    draw_quickfetch_items(d, (vec4){ 1, 1, 1, 0.75 }, (vec4){ 1, 1, 1, 0.9 });
    draw_quickfetch_text(d, 0, (vec4){ 1, 1, 1, 0.15 }, (vec4){ 1, 1, 1, 0.75 });
    pg_shader_begin(&d->core->shader_2d, NULL);
    pg_model_begin(&d->core->quad_2d_ctr, &d->core->shader_2d);
    pg_shader_2d_resolution(&d->core->shader_2d, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    if(looking_at_resource) {
        pg_shader_2d_texture(&d->core->shader_2d, &d->core->item_tex);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.75 }, (vec4){});
        pg_shader_2d_tex_frame(&d->core->shader_2d, 200 + looked_resource * 2);
        pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ 2, 2 }, (vec2){});
        pg_shader_2d_transform(&d->core->shader_2d,
            (vec2){ screen_pos[0], screen_pos[1] - 0.075 },
            (vec2){ 0.05, 0.05 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    } else if(looking_at_upgrade) {
        pg_shader_2d_texture(&d->core->shader_2d, &d->core->upgrades_tex);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.75 }, (vec4){});
        pg_shader_2d_tex_frame(&d->core->shader_2d, looked_upgrade[0]);
        pg_shader_2d_transform(&d->core->shader_2d,
            (vec2){ screen_pos[0] - 0.06, screen_pos[1] - 0.075 },
            (vec2){ 0.05, 0.05 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_tex_frame(&d->core->shader_2d, looked_upgrade[1]);
        pg_shader_2d_transform(&d->core->shader_2d,
            (vec2){ screen_pos[0] + 0.06, screen_pos[1] - 0.075 },
            (vec2){ 0.05, 0.05 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_texture(&d->core->shader_2d, &d->core->item_tex);
    pg_model_begin(&d->core->quad_2d_ctr, &d->core->shader_2d);
    pg_shader_2d_tex_frame(&d->core->shader_2d, 58);
    pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ 6, 6 }, (vec2){});
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.5 * ar, 0.5 }, (vec2){ 0.025, 0.025 }, 0);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.5 }, (vec4){});
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    /*  Health bar  */
    /*  Transparent backing */
    pg_model_begin(&d->core->quad_2d, &d->core->shader_2d);
    pg_shader_2d_tex_frame(&d->core->shader_2d, 232);
    pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ 3, 2 }, (vec2){ 0, 0 });
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.05, 0.75 }, (vec2){ 0.3, 0.2 }, 0);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.1 }, (vec4){});
    pg_model_draw(&d->core->quad_2d, NULL);
    /*  Actual health bar   */
    float hp_frac = (float)d->plr.HP / 100.0f;
    float hp_offset = (1 - hp_frac) * (24.0f / 256.0f);
    pg_shader_2d_tex_frame(&d->core->shader_2d, 232);
    pg_shader_2d_add_tex_tx(&d->core->shader_2d, (vec2){ hp_frac * 3, 2 }, (vec2){ hp_offset, 0 });
    pg_shader_2d_transform(&d->core->shader_2d,
                           (vec2){ 0.05 + 0.15 * (1 - hp_frac), 0.75 },
                           (vec2){ 0.3 * hp_frac, 0.2 }, 0);
    if(hp_frac < 0.5) {
        float red_frac = (hp_frac / 0.5);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, red_frac, red_frac, 1 }, (vec4){});
    } else pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_model_draw(&d->core->quad_2d, NULL);
    /*  Vignette effects    */
    if(d->teleport_ticks > 0) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){}, (vec2){ ar, 1 }, 0);
        pg_shader_2d_texture(&d->core->shader_2d, &d->core->radial_vignette);
        float hurt_alpha = (float)d->teleport_ticks / 240 * 0.5;
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 0, 0.5, 0, hurt_alpha }, (vec4){});
        pg_model_draw(&d->core->quad_2d, NULL);
    } else if(d->plr.pain_ticks > 0) {
        d->plr.pain_ticks = MIN(d->plr.pain_ticks, 240);
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
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.4 }, (vec4){});
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int scan_level = get_upgrade_level(d, BORK_UPGRADE_SCANNING);
    int translate = 0;
    if(dp->lines_translated && scan_level == 1) translate = 1;
    struct pg_shader_text text = { .use_blocks = 1 };
    int len = snprintf(text.block[0], 64, "%s", translate ? dp->title_translated : dp->title);
    vec4_set(text.block_style[0], ar * 0.5 - (len * 0.025 * 1.125 * 0.5), 0.95, 0.025, 1.125);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    float text_size = 0.015 * ar;
    float line_y = (float)d->hud_datapad_ticks / 190.0f;
    int lines = translate ? dp->lines_translated : dp->lines;
    int first_line = floor(line_y);
    int i, line;
    for(i = 0, line = first_line; i < 4 && line < lines; ++i, ++line) {
        if(line >= lines) break;
        if(line < 0) continue;
        len = snprintf(text.block[i + 1], 64, "%s",
                       translate ? dp->text_translated[line] : dp->text[line]);
        float dist_from_ctr = (float)line * 190.0f - d->hud_datapad_ticks - 190.0f;
        dist_from_ctr /= 380.0f;
        vec4_set(text.block_style[i + 1], ar * 0.5 - (len * text_size * 1.125 * 0.5),
                                      0.7 + dist_from_ctr * 0.08, text_size, 1.125);
        vec4_set(text.block_color[i + 1], 1, 1, 1, 1 - fabs(dist_from_ctr));
    }
    text.use_blocks = i + 2;
    pg_shader_text_write(shader, &text);
}

static void draw_weapon(struct bork_play_data* d, float hud_anim_lerp,
                        vec3 pos_lerp, vec2 dir_lerp)
{
    struct pg_shader* shader = &d->core->shader_3d;
    struct pg_model* model = &d->core->gun_model;
    struct bork_entity* held_item;
    const struct bork_entity_profile* prof;
    if(d->held_item < 0) return;
    if(d->switch_ticks >= 30) {
        if(d->switch_start_idx == -1) return;
        held_item = bork_entity_get(d->inventory.data[d->switch_start_idx]);
    } else held_item = bork_entity_get(d->inventory.data[d->held_item]);
    if(!held_item) return;
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
    } else if(d->switch_ticks) {
        int down_time = 60 - d->switch_ticks;
        float t = 1;
        if(down_time < 31) {
            t = (float)down_time / 30;
        } else if(d->switch_ticks < 31) {
            t = (float)d->switch_ticks / 30;
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
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, pos_lerp, d->plr.pos);
            if(ent->flags & BORK_ENTFLAG_ON_FIRE) {
                struct pg_light new_light = {};
                vec3 fire_pos;
                vec3_set_len(fire_pos, ent_to_plr, 0.25);
                vec3_sub(fire_pos, pos_lerp, fire_pos);
                pg_light_pointlight(&new_light, fire_pos, 2.5, (vec3){ 2.0, 1.5, 0 });
                ARR_PUSH(d->lights_buf, new_light);
            }
            float dir_adjust = 1.0f / (float)prof->dir_frames * 0.5;
            float angle = M_PI - atan2(ent_to_plr[0], ent_to_plr[1]) - ent->dir[0];
            angle = FMOD(angle, M_PI * 2);
            float angle_f = angle / (M_PI * 2) - dir_adjust;
            angle_f = 1.0f - FMOD(angle_f, 1.0f);
            int frame = (int)(angle_f * (float)prof->dir_frames);
            if(ent->type == BORK_ENEMY_GREAT_BANE
            || ent->type == BORK_ENEMY_LAIKA) frame *= 2;
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
            if(ent->type == BORK_ENTITY_DESKLAMP) {
                struct pg_light new_light = {};
                pg_light_pointlight(&new_light, pos_lerp, 2, (vec3){ 0.9, 0.5, 0.5 });
                ARR_PUSH(d->lights_buf, new_light);
            } else if(ent->type == BORK_ENTITY_BARREL) {
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
            vec3_add(pos_lerp, (vec3){}, ent->pos);
            mat4 transform;
            mat4_translate(transform, pos_lerp[0], pos_lerp[1], pos_lerp[2] + 0.05);
            if(ent_id == d->looked_item
            && vec3_dist2(ent->pos, d->plr.pos) < (2.5 * 2.5)) {
                pg_shader_sprite_color_mod(shader, (vec4){ 2.0f, 2.0f, 2.0f, 1.0f });
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
    vec3 plr_pos;
    vec3_mul(plr_pos, d->plr.pos, (vec3){ 1, 1, 2 });
    ARR_FOREACH_PTR(d->map.lights, light, i) {
        vec3 light_pos;
        vec3_mul(light_pos, light->pos, (vec3){ 1, 1, 2 });
        if(vec3_dist2(light_pos, plr_pos) > (48 * 48)) continue;
        ARR_PUSH(d->lights_buf, *light);
    }
    ARR_FOREACH_PTR(d->map.spotlights, light, i) {
        vec3 light_pos;
        vec3_mul(light_pos, light->pos, (vec3){ 1, 1, 2 });
        if(vec3_dist2(light_pos, plr_pos) > (48 * 48)) continue;
        ARR_PUSH(d->spotlights, *light);
    }
}

static void draw_fires(struct bork_play_data* d)
{
    struct pg_light light;
    pg_light_pointlight(&light, (vec3){}, 2.5, (vec3){ 1.0, 0.75, 0 });
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
            float light_size = part->light[3];
            if(part->flags & BORK_PARTICLE_LIGHT_DECAY) {
                light_size *= (float)part->ticks_left / (float)part->lifetime;
            } else if(part->flags & BORK_PARTICLE_LIGHT_EXPAND) {
                light_size *= 1 - (float)part->ticks_left / (float)part->lifetime;
            }
            pg_light_pointlight(&light, part->pos, light_size, part->light);
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
    struct pg_shader_text text = { .use_blocks = 4,
        .block = { "YOU GOT", "PUT DOWN", "RETURN TO MAIN MENU", "LOAD LAST SAVE GAME" },
        .block_style = {
            { ctr - (7 * 0.05 * 1.25 * 0.5), 0.35, 0.05, 1.25 },
            { ctr - (8 * 0.075 * 1.25 * 0.5), 0.41, 0.075, 1.25 },
            { ctr - (strlen(text.block[2]) * 0.04 * 1.25 * 0.5), 0.575, 0.04, 1.25 },
            { ctr - (strlen(text.block[3]) * 0.04 * 1.25 * 0.5), 0.675, 0.04, 1.25 } },
        .block_color = { { 1, 1, 1, 1 }, { 1, 1, 1, 1 },
                         { 1, 1, 1, d->menu.gameover.opt_idx == 0 ? 1 : 0.5f },
                         { 1, 1, 1, d->menu.gameover.opt_idx == 1 ? 1 : 0.5f } } };
    if(d->core->save_files.len) {
        int len = snprintf(text.block[4], 32, "%s", d->core->save_files.data[0].name);
        vec4_set(text.block_style[4], ctr - (len * 0.04 * 1.25 * 0.5), 0.725, 0.04, 1.25);
        vec4_set(text.block_color[4], 1, 1, 1, d->menu.gameover.opt_idx == 1 ? 1 : 0.5);
        text.use_blocks = 5;
    }
    pg_shader_text_write(shader, &text);
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
    r += sizeof(uint32_t) * fwrite(&d->tutorial_msg_seen, sizeof(uint32_t), 1, f);
    r += sizeof(int) * fwrite(&d->flashlight_on, sizeof(int), 1, f);
    r += sizeof(int) * fwrite(d->held_datapads, sizeof(int), NUM_DATAPADS, f);
    r += sizeof(int) * fwrite(&d->num_held_datapads, sizeof(int), 1, f);
    r += sizeof(uint32_t) * fwrite(&d->held_schematics, sizeof(uint32_t), 1, f);
    r += sizeof(enum bork_upgrade) * fwrite(d->upgrades, sizeof(enum bork_upgrade), 4, f);
    r += sizeof(int) * fwrite(d->upgrade_level, sizeof(int), 4, f);
    r += sizeof(int) * fwrite(d->upgrade_counters, sizeof(int), 4, f);
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
    r += fwrite(&d->killed_laika, sizeof(int), 1, f);
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
    fclose(f);
    bork_read_saves(d->core);
}

void load_game(struct bork_play_data* d, char* name)
{
    bork_play_reset(d);
    char filename[1024];
    snprintf(filename, 1024, "%ssaves/%s", d->core->base_path, name);
    FILE* f = fopen(filename, "rb");
    if(!f) {
        printf("Could not open save file %s\n", filename);
        return;
    }
    pg_mouse_mode(1);
    d->menu.state = BORK_MENU_CLOSED;
    bork_map_reset(&d->map);
    ARR_TRUNCATE_CLEAR(d->bullets, 0);
    ARR_TRUNCATE_CLEAR(d->particles, 0);
    size_t r = 0;
    /*  Player data */
    r += sizeof(struct bork_entity) * fread(&d->plr, sizeof(struct bork_entity), 1, f);
    r += sizeof(uint32_t) * fread(&d->tutorial_msg_seen, sizeof(uint32_t), 1, f);
    r += sizeof(int) * fread(&d->flashlight_on, sizeof(int), 1, f);
    r += sizeof(int) * fread(d->held_datapads, sizeof(int), NUM_DATAPADS, f);
    r += sizeof(int) * fread(&d->num_held_datapads, sizeof(int), 1, f);
    r += sizeof(uint32_t) * fread(&d->held_schematics, sizeof(uint32_t), 1, f);
    r += sizeof(enum bork_upgrade) * fread(d->upgrades, sizeof(enum bork_upgrade), 4, f);
    r += sizeof(int) * fread(d->upgrade_level, sizeof(int), 4, f);
    r += sizeof(int) * fread(d->upgrade_counters, sizeof(int), 4, f);
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
    r += fread(&d->killed_laika, sizeof(int), 1, f);
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
    /*  Fire objects (ignored in the save file for compatibility...)    */
    r += sizeof(uint32_t) * fread(&len, sizeof(uint32_t), 1, f);
    fseek(f, sizeof(struct bork_map_object) * len, SEEK_CUR);
    //ARR_RESERVE_CLEAR(d->map.fire_objs, len);
    //r += sizeof(struct bork_map_object) * fread(d->map.fire_objs.data, sizeof(struct bork_map_object), len, f);
    //d->map.fire_objs.len = len;
    //ARR_TRUNCATE(d->map.fire_objs, len);
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
    d->map.plr = &d->plr;
    fclose(f);
    bork_play_reset_hud_anim(d);
    int plr_x = floor(d->plr.pos[0] / 2);
    int plr_y = floor(d->plr.pos[1] / 2);
    int plr_z = floor(d->plr.pos[2] / 2);
    d->plr_map_pos[0] = plr_x;
    d->plr_map_pos[1] = plr_y;
    d->plr_map_pos[2] = plr_z;
    if(!d->decoy_active) bork_map_build_plr_dist(&d->map, d->plr.pos);
    pg_audio_channel_pause(1, 0);
    pg_audio_channel_pause(2, 0);
    pg_reset_input();
    char announcement[64];
    snprintf(announcement, 64, "LOADED SAVE: %s", name);
    hud_announce(d, announcement);
}

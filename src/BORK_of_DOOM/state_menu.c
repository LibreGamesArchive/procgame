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

enum bork_menu_option {
    BORK_MENU_NEW_GAME,
    BORK_MENU_CONTINUE,
    BORK_MENU_OPTIONS,
    BORK_MENU_CREDITS,
    BORK_MENU_EXIT,
    BORK_MENU_EDITOR,
    BORK_MENU_COUNT
};

struct bork_menu_data {
    struct bork_game_core* core;
    const uint8_t* kb_state;
    vec2 mouse_motion;
    int current_selection;
    enum { BORK_MENU_BASE, BORK_MENU_SHOW_CREDITS,
           BORK_MENU_SELECT_SAVE, BORK_MENU_SHOW_OPTIONS } mode;
    int save_idx, save_scroll, save_side, save_action_idx;
    int opt_idx, opt_scroll;
    int delete_ticks;
    int remap_ctrl;
    int opt_fullscreen;
    int opt_res[2];
    char opt_res_input[8];
    int opt_res_input_idx;
    int opt_res_typing;
    int core_res;
};

static void bork_menu_tick(struct pg_game_state* state);
static void bork_menu_draw(struct pg_game_state* state);

void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 30, 3);
    struct bork_menu_data* d = malloc(sizeof(*d));
    *d = (struct bork_menu_data) {
        .core = core,
        .kb_state = SDL_GetKeyboardState(NULL) };
    //SDL_SetRelativeMouseMode(SDL_TRUE);
    /*  Assign all the pointers, and it's finished  */
    d->opt_fullscreen = core->fullscreen;
    d->opt_res[0] = core->screen_size[0];
    d->opt_res[1] = core->screen_size[1];
    state->data = d;
    state->update = NULL;
    state->tick = bork_menu_tick;
    state->draw = bork_menu_draw;
    state->deinit = free;
}

/*
static const resolutions_4_3[11][2] = {
    { 640, 480 }, { 800, 600 }, { 960, 720 }, { 1024, 768 }, { 1280, 960 },
    { 1400, 1050 }, { 1440, 1080 }, { 1600, 1200 }, { 1856, 1392 },
    { 1920, 1440 }, { 2048, 1536 } };

static const resolutions_16_9[8][2] = {
    { 960, 540 }, { 1024, 576 }, { 1280, 720 }, { 1600, 900 },
    { 1920, 1080 }, { 2560, 1440 }, { 3200, 1800 }, { 3840, 2160 } };

static const resolutions_16_10[7][2] = {
    { 960, 600 }, { 1024, 640 }, { 1280, 800 }, { 1440, 900 }, { 1680, 1050 },
    { 1920, 1200 }, { 2560, 1600 } };*/

static void bork_menu_tick(struct pg_game_state* state)
{
    struct bork_menu_data* d = state->data;
    uint8_t* kmap = d->core->ctrl_map;
    pg_poll_input();
    if(pg_user_exit()) state->running = 0;
    float ar = d->core->aspect_ratio;
    vec2 mouse_pos;
    vec2 mouse_motion;
    int click;
    int clicked_item = 0;
    pg_mouse_motion(mouse_motion);
    pg_mouse_pos(mouse_pos);
    vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0],
                                           1 / d->core->screen_size[1] });
    click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    if(d->mode == BORK_MENU_BASE) {
        int i;
        for(i = 0; i < 6; ++i) {
            if(mouse_pos[0] > ar * 0.55 && mouse_pos[0] < ar * 0.55 + 0.5
            && mouse_pos[1] > i * 0.1 + 0.205 && mouse_pos[1] < i * 0.1 + 0.285) {
                if(mouse_motion[0] != 0 || mouse_motion[1] != 0) {
                    if(i != d->current_selection) pg_audio_play(&d->core->menu_sound, 0.2);
                    d->current_selection = i;
                }
                if(click) {
                    d->current_selection = i;
                    clicked_item = 1;
                }
            }
        }
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            d->current_selection = MOD(d->current_selection + 1, BORK_MENU_COUNT);
            pg_audio_play(&d->core->menu_sound, 0.2);
        }
        if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            d->current_selection = MOD(d->current_selection - 1, BORK_MENU_COUNT);
            pg_audio_play(&d->core->menu_sound, 0.2);
        }
        if(clicked_item || pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
            if(d->current_selection == BORK_MENU_NEW_GAME) {
                bork_play_start(state, d->core);
            } else if(d->current_selection == BORK_MENU_CONTINUE) {
                d->mode = BORK_MENU_SELECT_SAVE;
                d->save_idx = 0;
                d->save_scroll = 0;
            } else if(d->current_selection == BORK_MENU_OPTIONS) {
                d->mode = BORK_MENU_SHOW_OPTIONS;
                d->opt_idx = 0;
                d->opt_scroll = 0;
            } else if(d->current_selection == BORK_MENU_CREDITS) {
                d->mode = BORK_MENU_SHOW_CREDITS;
            } else if(d->current_selection == BORK_MENU_EXIT) {
                state->running = 0;
            } else if(d->current_selection == BORK_MENU_EDITOR) {
                bork_editor_start(state, d->core);
            }
        }
    } else if(d->mode == BORK_MENU_SHOW_CREDITS) {
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            d->mode = BORK_MENU_BASE;
        } else if(click && fabs(mouse_pos[0] - ar * 0.655) < 0.2
               && fabs(mouse_pos[1] - 0.87) < 0.3) {
            d->mode = BORK_MENU_BASE;
        }
    } else if(d->mode == BORK_MENU_SELECT_SAVE) {
        int i;
        int num_saves = MIN(d->core->save_files.len + 1, 6);
        for(i = 0; i < num_saves; ++i) {
            if(click && mouse_pos[0] > ar * 0.55 && mouse_pos[0] < ar
            && mouse_pos[1] > (0.23 + 0.1 * i) && mouse_pos[1] < (0.3 + 0.1 * i)) {
                if(d->save_idx != d->save_scroll + i) pg_audio_play(&d->core->menu_sound, 0.2);
                d->save_idx = d->save_scroll + i;
                d->save_side = 1;
            }
        }
        if(fabs(mouse_pos[0] - ar * 0.35) < 0.2
        && fabs(mouse_pos[1] - 0.72) < 0.04) {
            d->save_action_idx = 0;
            d->save_side = 0;
            if(click) {
                d->mode = BORK_MENU_BASE;
                d->save_scroll = 0;
                d->save_idx = 0;
            }
        }
        if(fabs(mouse_pos[0] - ar * 0.35) < 0.2
        && fabs(mouse_pos[1] - 0.8) < 0.04) {
            d->save_action_idx = 1;
            d->save_side = 0;
        }
        if(click && fabs(mouse_pos[0] - ar * 0.655) < 0.2
        && fabs(mouse_pos[1] - 0.9) < 0.04) {
            bork_play_start(state, d->core);
            struct bork_play_data* play_d = state->data;
            struct bork_save* save = &d->core->save_files.data[d->save_idx];
            load_game(play_d, save->name);
            pg_flush_input();
            return;
        }
        if((d->core->save_files.len > 0 && d->save_action_idx == 1 && d->save_side == 0)
        && (click || pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HELD)
        || pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HELD))) {
            ++d->delete_ticks;
            if(d->delete_ticks >= 60) {
                bork_delete_save(d->core, d->save_idx);
                d->delete_ticks = 0;
                if(d->save_scroll + 6 > d->core->save_files.len) {
                    d->save_scroll = MAX(0, d->save_scroll - 1);
                }
            }
        } else d->delete_ticks = 0;
        if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            if(d->save_side == 0) {
                d->save_action_idx = 1;
            } else {
                d->save_idx = MIN(d->core->save_files.len - 1, d->save_idx + 1);
                if(d->save_scroll + 5 < d->save_idx) d->save_scroll = d->save_idx - 5;
                pg_audio_play(&d->core->menu_sound, 0.2);
            }
        } else if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            if(d->save_side == 0) {
                d->save_action_idx = 0;
            } else {
                d->save_idx = MAX(0, d->save_idx - 1);
                if(d->save_scroll > d->save_idx) d->save_scroll = d->save_idx;
                pg_audio_play(&d->core->menu_sound, 0.2);
            }
        } else if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)
               || stick_ctrl_x == -1) {
            d->save_side = 0;
            pg_audio_play(&d->core->menu_sound, 0.2);
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)
               || stick_ctrl_x == 1) {
            d->save_side = 1;
            pg_audio_play(&d->core->menu_sound, 0.2);
        } else if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
               || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
            if(d->save_side == 1) {
                bork_play_start(state, d->core);
                struct bork_play_data* play_d = state->data;
                struct bork_save* save = &d->core->save_files.data[d->save_idx];
                load_game(play_d, save->name);
                pg_flush_input();
                return;
            } else {
                if(d->save_action_idx == 0) {
                    d->mode = BORK_MENU_BASE;
                    d->save_side = 1;
                }
            }
        } else if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)
        && d->save_scroll > 0) {
            d->save_scroll = d->save_scroll - 1;
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
        && d->save_scroll + 6 < d->core->save_files.len) {
            d->save_scroll = d->save_scroll + 1;
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
               || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            d->mode = BORK_MENU_BASE;
            d->save_scroll = 0;
            d->save_idx = 0;
        }
    } else if(d->mode == BORK_MENU_SHOW_OPTIONS) {
        if(d->remap_ctrl) {
            uint8_t ctrl = pg_first_input();
            if(ctrl) {
                d->core->ctrl_map[d->opt_idx - 4] = ctrl;
                d->remap_ctrl = 0;
            }
        } else if(d->opt_res_typing) {
            if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
                d->opt_res_typing = 0;
                pg_text_mode(0);
            } else if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)) {
                int res_opt = d->opt_idx - 1;
                int input = (res_opt == 0) ? MAX(atoi(d->opt_res_input), 160)
                                         : MAX(atoi(d->opt_res_input), 90);
                d->opt_res[res_opt] = input;
                d->opt_res_typing = 0;
                d->opt_res_input_idx = 0;
                pg_text_mode(0);
            } else if(d->opt_res_input_idx > 0
                   && pg_check_input(SDL_SCANCODE_BACKSPACE, PG_CONTROL_HIT)) {
                d->opt_res_input[--d->opt_res_input_idx] = '\0';
            } else {
                int len = pg_copy_text_input(d->opt_res_input + d->opt_res_input_idx,
                                             6 - d->opt_res_input_idx);
                d->opt_res_input_idx += len;
            }
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
               || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            d->mode = BORK_MENU_BASE;
            d->opt_idx = 0;
            d->opt_scroll = 0;
            if(d->opt_fullscreen != d->core->fullscreen
            || d->opt_res[0] != d->core->screen_size[0]
            || d->opt_res[1] != d->core->screen_size[1]) {
                bork_reinit_gfx(d->core, d->opt_res[0], d->opt_res[1], d->opt_fullscreen);
            }
            bork_save_options(d->core);
        } else if(click) {
            if(fabs(mouse_pos[0] - ar * 0.85) < 0.2
            && fabs(mouse_pos[1] - 0.115) < 0.03) {
                d->mode = BORK_MENU_BASE;
            } else if(fabs(mouse_pos[0] - ar * 0.65) < 0.2
            && fabs(mouse_pos[1] - 0.115) < 0.03) {
                if(d->opt_fullscreen != d->core->fullscreen
                || d->opt_res[0] != d->core->screen_size[0]
                || d->opt_res[1] != d->core->screen_size[1]) {
                    bork_reinit_gfx(d->core, d->opt_res[0], d->opt_res[1], d->opt_fullscreen);
                }
                bork_save_options(d->core);
            } else {
                int i;
                for(i = 0; i < 10; ++i) {
                    if(fabs(mouse_pos[0] - (ar * 0.1 + 0.0125 + 0.25)) < 0.25
                    && fabs(mouse_pos[1] - (0.25 + i * 0.06 + 0.0125)) < 0.025) {
                        d->opt_idx = d->opt_scroll + i;
                    } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.1)) < 0.15
                           && fabs(mouse_pos[1] - (0.25 + i * 0.06 + 0.0125)) < 0.025) {
                        int opt = d->opt_scroll + i;
                        d->opt_idx = opt;
                        if(opt == 0) d->opt_fullscreen = 1 - d->opt_fullscreen;
                        else if(opt == 1 || opt == 2) {
                            d->opt_res_typing = 1;
                            d->opt_res_input_idx = 0;
                            pg_text_mode(1);
                        } else if(opt == 3) {
                            if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                d->core->mouse_sensitivity =
                                    MAX(0.0001, d->core->mouse_sensitivity - 0.0001);
                            } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.2)) < 0.03) {
                                d->core->mouse_sensitivity =
                                    MIN(5.0f / 1000, d->core->mouse_sensitivity + 0.0001);
                            }
                        } else if(opt == BORK_CTRL_COUNT + 4) {
                            bork_reset_keymap(d->core);
                            d->core->mouse_sensitivity = 1.0f / 1000;
                        } else if(opt > 3) {
                            d->remap_ctrl = 1;
                        }
                    }
                }
            }
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)) {
            if(d->opt_idx == 0) {
                d->opt_fullscreen = 1 - d->opt_fullscreen;
            } else if(d->opt_idx == 3) {
                d->core->mouse_sensitivity = MIN(5.0f / 1000, d->core->mouse_sensitivity + 0.0001);
            }
        } else if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)) {
            if(d->opt_idx == 0) {
                d->opt_fullscreen = 1 - d->opt_fullscreen;
            } else if(d->opt_idx == 3) {
                d->core->mouse_sensitivity = MAX(0.0001, d->core->mouse_sensitivity - 0.0001);
            }
        } else if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {

            if(d->opt_idx == 1 || d->opt_idx == 2) {
                d->opt_res_typing = 1;
                pg_text_mode(1);
            } else if(d->opt_idx == 0) {
                d->opt_fullscreen = 1 - d->opt_fullscreen;
            } else if(d->opt_idx == BORK_CTRL_COUNT + 4) {
                bork_reset_keymap(d->core);
                d->core->mouse_sensitivity = 1.0f / 1000;
            } else if(d->opt_idx > 3) d->remap_ctrl = 1;
        } else if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
               || stick_ctrl_y == 1) {
            d->opt_idx = MIN(BORK_CTRL_COUNT + 4, d->opt_idx + 1);
            if(d->opt_scroll + 9 < d->opt_idx) d->opt_scroll = d->opt_idx - 9;
            else if(d->opt_scroll > d->opt_idx) d->opt_scroll = d->opt_idx;
            pg_audio_play(&d->core->menu_sound, 0.2);
        } else if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
               || stick_ctrl_y == -1) {
            d->opt_idx = MAX(0, d->opt_idx - 1);
            if(d->opt_scroll + 9 < d->opt_idx) d->opt_scroll = d->opt_idx - 9;
            else if(d->opt_scroll > d->opt_idx) d->opt_scroll = d->opt_idx;
            pg_audio_play(&d->core->menu_sound, 0.2);
        } else if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)
        && d->opt_scroll > 0) {
            d->opt_scroll -= 1;
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
        && d->opt_scroll + 10 < BORK_CTRL_COUNT + 5) {
            d->opt_scroll += 1;
        }
    }
    pg_flush_input();
}

static void bork_menu_draw(struct pg_game_state* state)
{
    struct bork_menu_data* d = state->data;
    float ar = d->core->aspect_ratio;
    /*  Overlay */
    pg_screen_dst();
    struct pg_shader* shader = &d->core->shader_text;
    bork_draw_backdrop(d->core, (vec4){ 2, 2, 2, 1 },
                       (float)state->ticks / (float)state->tick_rate);
    bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 1 });
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){d->core->aspect_ratio, 1});
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    static const char* menu_opts[] = {
        "NEW GAME", "CONTINUE", "OPTIONS", "CREDITS", "EXIT", "EDITOR" };
    struct pg_shader_text text = {
        .use_blocks = 9,
        .block = { "BORK", "OF", "DOOM!", },
        .block_style = {
            /*  BORK OF DOOOOOM */
            { ar * 0.25 - (4 * 0.1 * 1.1 * 0.5), 0.45, 0.1, 1.1 },
            { ar * 0.25 - (2 * 0.025 * 1.1 * 0.5) - 0.17, 0.58, 0.025, 1.1 },
            { ar * 0.25 - (5 * 0.06 * 1.1 * 0.5) + 0.07, 0.56, 0.06, 1.1 },
        },
        .block_color = { { 1, 1, 1, 0.7 }, { 1, 1, 1, 0.6 }, { 1, 0, 0, 1 }, },
    };
    if(d->mode == BORK_MENU_BASE) {
        int i;
        for(i = 0; i < BORK_MENU_COUNT; ++i) {
            strncpy(text.block[i + 3], menu_opts[i], 10);
            vec4_set(text.block_style[i + 3],
                (d->current_selection == i) * 0.075 + ar * 0.55,
                i * 0.1 + 0.225, 0.04, 1.2);
            vec4_set(text.block_color[i + 3], 1, 1, 1, 1);
        }
    } else if(d->mode == BORK_MENU_SELECT_SAVE) {
        int ti = 2;
        int i;
        int num_saves = MIN(d->core->save_files.len, 6);
        int len = snprintf(text.block[++ti], 32, "BACK");
        vec4_set(text.block_style[ti], ar * 0.35 - (4 * 0.04 * 1.25 * 0.5),
                 0.7, 0.04, 1.2);
        if(d->save_action_idx == 0) {
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else {
            vec4_set(text.block_color[ti], 1, 1, 1, d->save_side == 0 ? 0.5 : 1);
        }
        len = snprintf(text.block[++ti], 32, "DELETE");
        vec4_set(text.block_style[ti], ar * 0.35 - (6 * 0.04 * 1.25 * 0.5),
                 0.8, 0.04, 1.2);
        if(d->delete_ticks) {
            text.block_style[ti][0] += (RANDF * 0.1 - 0.05) * (d->delete_ticks / 60.0f);
            text.block_style[ti][1] += (RANDF * 0.1 - 0.05) * (d->delete_ticks / 60.0f);
        }
        if(d->save_action_idx == 1) {
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else {
            vec4_set(text.block_color[ti], 1, 1, 1, d->save_side == 0 ? 0.75 : 0.5);
        }
        len = snprintf(text.block[++ti], 32, "LOAD");
        vec4_set(text.block_style[ti], ar * 0.655 - (4 * 0.05 * 1.25 * 0.5),
                 0.9, 0.05, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        for(i = 0; i < num_saves; ++i) {
            int is_selected = (i + d->save_scroll == d->save_idx);
            struct bork_save* save = &d->core->save_files.data[i + d->save_scroll];
            len = snprintf(text.block[++ti], 32, "%s", save->name);
            vec4_set(text.block_style[ti], ar * 0.55 + 0.04 * is_selected,
                     0.25 + 0.1 * i, 0.03, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        }
        text.use_blocks = ti + num_saves;
    } else if(d->mode == BORK_MENU_SHOW_CREDITS) {
        int ti = 0;
        float y = 0.25;
        int len;
        len = snprintf(text.block[ti], 64, "DEVELOPED BY");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.02 * 1.25 * 0.5), y, 0.02, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        y += 0.04;
        len = snprintf(text.block[++ti], 64, "JOSHUA GILES");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.04 * 1.25 * 0.5), y, 0.04, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        y += 0.1;
        len = snprintf(text.block[++ti], 64, "TESTING AND DESIGN ADVICE FROM");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.02 * 1.25 * 0.5), y, 0.02, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        y += 0.04;
        len = snprintf(text.block[++ti], 64, "KDRNIC");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.04 * 1.25 * 0.5), y, 0.04, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        y += 0.1;
        len = snprintf(text.block[++ti], 64, "SUPER SPECIAL THANKS TO");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.02 * 1.25 * 0.5), y, 0.02, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        y += 0.04;
        len = snprintf(text.block[++ti], 64, "CAIN, STACY, AND");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.04 * 1.25 * 0.5), y, 0.04, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        y += 0.06;
        len = snprintf(text.block[++ti], 64, "MY SISTER KAITLYN");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.04 * 1.25 * 0.5), y, 0.04, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        y += 0.06;
        len = snprintf(text.block[++ti], 64, "WHOSE SUPPORT MADE THIS");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.02 * 1.25 * 0.5), y, 0.02, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        y += 0.04;
        len = snprintf(text.block[++ti], 64, "GAME POSSIBLE IN THE FIRST PLACE");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.02 * 1.25 * 0.5), y, 0.02, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.75);
        y += 0.05;
        len = snprintf(text.block[++ti], 64, "AND KARL MARX");
        vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.01 * 1.25 * 0.5), y, 0.01, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        len = snprintf(text.block[++ti], 64, "BACK");
        vec4_set(text.block_style[ti], ar * 0.655 - (len * 0.04 * 1.25 * 0.5), 0.85, 0.04, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        text.use_blocks = ti + 1;
    } else if(d->mode == BORK_MENU_SHOW_OPTIONS) {
        int ti = 0;
        int len = snprintf(text.block[ti], 64, "OPTIONS");
        vec4_set(text.block_style[ti], 0.1, 0.1, 0.05, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 0.7);
        len = snprintf(text.block[++ti], 64, "APPLY");
        vec4_set(text.block_style[ti], ar * 0.65 - (len * 0.03 * 1.25 * 0.5), 0.1, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        len = snprintf(text.block[++ti], 64, "BACK");
        vec4_set(text.block_style[ti], ar * 0.85 - (len * 0.03 * 1.25 * 0.5), 0.1, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        int i;
        for(i = 0; i < 10; ++i) {
            int opt = d->opt_scroll + i;
            int is_selected = (d->opt_scroll + i == d->opt_idx);
            if(opt == 0) {
                strncpy(text.block[++ti], "FULLSCREEN", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, d->opt_fullscreen ? "YES" : "NO");
            } else if(opt == 1) {
                strncpy(text.block[++ti], "RES. WIDTH", 32);
                memset(text.block[ti + 1], 0, 32);
                if(d->opt_res_typing && d->opt_idx == opt) {
                    snprintf(text.block[ti + 1], 32, ">%s", d->opt_res_input);
                } else {
                    snprintf(text.block[ti + 1], 32, "%d", d->opt_res[0]);
                }
            } else if(opt == 2) {
                strncpy(text.block[++ti], "RES. HEIGHT", 32);
                memset(text.block[ti + 1], 0, 32);
                if(d->opt_res_typing && d->opt_idx == opt) {
                    snprintf(text.block[ti + 1], 32, ">%s", d->opt_res_input);
                } else {
                    snprintf(text.block[ti + 1], 32, "%d", d->opt_res[1]);
                }
            } else if(opt == 3) {
                strncpy(text.block[++ti], "MOUSE SENSITIVITY", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "< %.1f >", d->core->mouse_sensitivity * 1000);
            } else if(opt == BORK_CTRL_COUNT + 4) {
                strncpy(text.block[++ti], "RESET TO DEFAULTS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "[RESET]");
            } else {
                strncpy(text.block[++ti], bork_ctrl_names[opt - 4], 32);
                strncpy(text.block[ti + 1], pg_input_name(d->core->ctrl_map[opt - 4]), 32);
            }
            vec4_set(text.block_style[ti], ar * 0.1 + is_selected * 0.05,
                                           0.25 + i * 0.06, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            vec4_set(text.block_style[ti + 1], ar * 0.65, 0.25 + i * 0.06, 0.025, 1.2);
            vec4_set(text.block_color[ti + 1], 1, 1, 1, 1);
            ti += 1;
        }
        if(d->remap_ctrl) {
            len = snprintf(text.block[++ti], 64, "PRESS DESIRED CONTROL FOR");
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            len = snprintf(text.block[++ti], 64, "%s", bork_ctrl_names[d->opt_idx - 4]);
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.925, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->opt_idx > 3) {
            len = snprintf(text.block[++ti], 64, "PRESS %s TO RE-MAP CONTROL",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->opt_idx == 0 || d->opt_idx == 3) {
            len = snprintf(text.block[++ti], 64, "LEFT/RIGHT TO SET OPTION");
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->opt_idx == 1 || d->opt_idx == 2) {
            len = snprintf(text.block[++ti], 64, "PRESS %s TO SET RESOLUTION",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        }
        text.use_blocks = ti + 1;
    }
    pg_shader_text_write(shader, &text);
    pg_shader_text_resolution(shader, d->core->screen_size);
    bork_draw_fps(d->core);
    shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    if(d->mode == BORK_MENU_SELECT_SAVE) {
        if(d->save_scroll > 0) {
            pg_shader_2d_tex_frame(shader, 198);
            pg_shader_2d_transform(shader, (vec2){ ar * 0.5, 0.25 }, (vec2){ 0.04, 0.04 }, 0);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        if(d->save_scroll + 6 < d->core->save_files.len) {
            pg_shader_2d_tex_frame(shader, 199);
            pg_shader_2d_transform(shader, (vec2){ ar * 0.5, 0.75 }, (vec2){ 0.04, 0.04 }, 0);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
    }
}

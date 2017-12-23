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

void tick_game_menu(struct bork_play_data* d, struct pg_game_state* state)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    float ar = d->core->aspect_ratio;
    vec2 mouse_pos;
    int click;
    pg_mouse_pos(mouse_pos);
    vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0],
                                           1 / d->core->screen_size[1] });
    click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    int mouse_over_option = 0;
    int i;
    float opt_size[6] = { 0.5, 0.7, 0.7, 0.8, 0.9, 0.7 };
    for(i = 0; i < 6; ++i) {
        if(mouse_pos[0] > 0.1 && mouse_pos[0] < opt_size[i]
        && mouse_pos[1] > (0.25 + 0.1 * i) && mouse_pos[1] < (0.25 + 0.1 * i + 0.05)) {
            if(d->menu.game.mode == GAME_MENU_BASE) {
                d->menu.game.selection_idx = i;
                mouse_over_option = 1;
            } else if(d->menu.game.mode != GAME_MENU_SHOW_OPTIONS && click) {
                d->menu.game.mode = GAME_MENU_BASE;
            }
        }
    }
    if(d->menu.game.mode == GAME_MENU_SELECT_SAVE
    || d->menu.game.mode == GAME_MENU_LOAD) {
        int num_saves = MIN(d->core->save_files.len, 6);
        for(i = 0; i < num_saves; ++i) {
            if(mouse_pos[0] > ar * 0.55 && mouse_pos[0] < ar
            && mouse_pos[1] > (0.25 + 0.1 * i) && mouse_pos[1] < (0.28 + 0.1 * i)) {
                d->menu.game.save_idx = d->menu.game.save_scroll + i;
                if(click && d->menu.game.mode == GAME_MENU_LOAD) {
                    struct bork_save* save = &d->core->save_files.data[d->menu.game.save_idx];
                    load_game(d, save->name);
                    d->menu.game.mode = GAME_MENU_BASE;
                } else if(click && d->menu.game.mode == GAME_MENU_SELECT_SAVE) {
                    if(d->menu.game.save_idx == -1) {
                        d->menu.game.save_name_len = snprintf(d->menu.game.save_name, 32, "SAVE_%03d",
                                                              (int)d->core->save_files.len);
                        d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                        pg_text_mode(1);
                    } else {
                        d->menu.game.mode = GAME_MENU_CONFIRM_OVERWRITE;
                    }
                }
            }
        }
        int s = (d->menu.game.mode == GAME_MENU_SELECT_SAVE) ? -1 : 0;
        if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)
        && d->menu.game.save_scroll > s) {
            d->menu.game.save_scroll = d->menu.game.save_scroll - 1;
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
        && d->menu.game.save_scroll + 6 < d->core->save_files.len) {
            d->menu.game.save_scroll = d->menu.game.save_scroll + 1;
        }
    } else if(d->menu.game.mode == GAME_MENU_CONFIRM_OVERWRITE) {
        if(fabs(mouse_pos[0] - (ar * 0.465)) < 0.04
        && fabs(mouse_pos[1] - 0.9) < 0.02) {
            if(click) {
                struct bork_save* save = &d->core->save_files.data[d->menu.game.save_idx];
                save_game(d, save->name);
                d->menu.game.mode = GAME_MENU_BASE;
            }
            d->menu.game.confirm_opt = 0;
        } else if(fabs(mouse_pos[0] - (ar * 0.565)) < 0.04
        && fabs(mouse_pos[1] - 0.9) < 0.02) {
            if(click) {
                if(d->menu.game.save_idx == -1) {
                    d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                    pg_text_mode(1);
                }
                d->menu.game.mode = GAME_MENU_SELECT_SAVE;
            }
            d->menu.game.confirm_opt = 1;
        }
    }
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    if(d->menu.game.mode == GAME_MENU_BASE) {
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.state = BORK_MENU_CLOSED;
            SDL_ShowCursor(SDL_DISABLE);
            pg_mouse_mode(1);
            pg_audio_channel_pause(1, 0);
            pg_audio_channel_pause(2, 0);
        }
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.selection_idx = MIN(d->menu.game.selection_idx + 1, 5);
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.selection_idx = MAX(d->menu.game.selection_idx - 1, 0);
        }
        if((click && mouse_over_option)
        || pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            switch(d->menu.game.selection_idx) {
                case 0:
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    d->menu.state = BORK_MENU_CLOSED;
                    pg_mouse_mode(1);
                    pg_audio_channel_pause(1, 0);
                    pg_audio_channel_pause(2, 0);
                    SDL_ShowCursor(SDL_DISABLE);
                    pg_reset_input();
                    break;
                case 1: {
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    d->menu.game.mode = GAME_MENU_SELECT_SAVE;
                    d->menu.game.save_scroll = -1;
                    d->menu.game.save_idx = -1;
                    d->menu.game.save_name_len = 0;
                    snprintf(d->menu.game.save_name, 32, "NEW SAVE");
                    break;
                } case 2: {
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    d->menu.game.mode = GAME_MENU_LOAD;
                    d->menu.game.save_scroll = 0;
                    d->menu.game.save_idx = 0;
                    break;
                } case 3: {
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    d->menu.game.mode = GAME_MENU_SHOW_OPTIONS;
                    d->menu.game.opt_res[0] = d->core->screen_size[0];
                    d->menu.game.opt_res[1] = d->core->screen_size[1];
                    d->menu.game.opt_fullscreen = d->core->fullscreen;
                    break;
                } case 4: {
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    struct bork_game_core* core = d->core;
                    bork_play_deinit(d);
                    bork_menu_start(state, core);
                    pg_flush_input();
                    return;
                } case 5: state->running = 0; break;
            }
        }
    } else if(d->menu.game.mode == GAME_MENU_LOAD) {
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.mode = GAME_MENU_BASE;
            d->menu.game.save_idx = 0;
        }
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.save_idx = MIN(d->menu.game.save_idx + 1, d->core->save_files.len - 1);
            if(d->menu.game.save_idx >= d->menu.game.save_scroll + 6) {
                ++d->menu.game.save_scroll;
            }
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.save_idx = MAX(d->menu.game.save_idx - 1, 0);
            if(d->menu.game.save_idx < d->menu.game.save_scroll) {
                --d->menu.game.save_scroll;
            }
        }
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            struct bork_save* save = &d->core->save_files.data[d->menu.game.save_idx];
            load_game(d, save->name);
            d->menu.game.mode = GAME_MENU_BASE;
        }
    } else if(d->menu.game.mode == GAME_MENU_SELECT_SAVE) {
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.mode = GAME_MENU_BASE;
            d->menu.game.save_idx = 0;
            d->menu.game.save_name_len = 0;
            d->menu.game.save_scroll = 0;
        }
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.save_idx = MIN(d->menu.game.save_idx + 1, d->core->save_files.len - 1);
            if(d->menu.game.save_idx >= d->menu.game.save_scroll + 6) {
                ++d->menu.game.save_scroll;
            }
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.save_idx = MAX(d->menu.game.save_idx - 1, -1);
            if(d->menu.game.save_idx < d->menu.game.save_scroll) {
                --d->menu.game.save_scroll;
            }
        }
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.game.save_idx == -1) {
                d->menu.game.save_name_len = snprintf(d->menu.game.save_name, 32, "SAVE_%03d",
                                                      (int)d->core->save_files.len);
                d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                pg_text_mode(1);
            } else {
                d->menu.game.mode = GAME_MENU_CONFIRM_OVERWRITE;
            }
        }
    } else if(d->menu.game.mode == GAME_MENU_EDIT_SAVE) {
        int len = pg_copy_text_input(d->menu.game.save_name + d->menu.game.save_name_len,
                                     32 - d->menu.game.save_name_len);
        if(len) {
            len += d->menu.game.save_name_len;
            int save_i;
            for(save_i = d->menu.game.save_name_len; save_i < len; ++save_i) {
                if(!isalnum(d->menu.game.save_name[save_i])
                && d->menu.game.save_name[save_i] != '_'
                && d->menu.game.save_name[save_i] != ' ') {
                    int inner_i;
                    for(inner_i = save_i; inner_i < len; ++inner_i) {
                        d->menu.game.save_name[inner_i] = d->menu.game.save_name[inner_i] + 1;
                    }
                    --len;
                }
            }
            d->menu.game.save_name_len = len;
        }
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.mode = GAME_MENU_SELECT_SAVE;
            pg_text_mode(0);
        } else if(pg_check_keycode(SDLK_BACKSPACE, PG_CONTROL_HIT)) {
            if(d->menu.game.save_name_len > 0) {
                d->menu.game.save_name[--d->menu.game.save_name_len] = '\0';
            }
        } else if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            int i;
            struct bork_save* save;
            ARR_FOREACH_PTR(d->core->save_files, save, i) {
                if(strncmp(save->name, d->menu.game.save_name, 32) == 0) {
                    d->menu.game.mode = GAME_MENU_CONFIRM_OVERWRITE;
                    pg_text_mode(0);
                }
            }
            struct bork_save new_save;
            strncpy(new_save.name, d->menu.game.save_name, 32);
            ARR_PUSH(d->core->save_files, new_save);
            save_game(d, new_save.name);
            d->menu.game.save_idx = 0;
            d->menu.game.save_scroll = 0;
            d->menu.game.mode = GAME_MENU_BASE;
        }
    } else if(d->menu.game.mode == GAME_MENU_CONFIRM_OVERWRITE) {
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.game.save_idx == -1) {
                d->menu.game.save_name_len = snprintf(d->menu.game.save_name, 32, "SAVE_%03d",
                                                      (int)d->core->save_files.len);
                d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                pg_text_mode(1);
            }
            d->menu.game.mode = GAME_MENU_SELECT_SAVE;
        }
        if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)
        || stick_ctrl_x == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.confirm_opt = 0;
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)
        || stick_ctrl_x == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.game.confirm_opt = 1;
        } 
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.game.confirm_opt == 1) {
                if(d->menu.game.save_idx == -1) {
                    d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                    pg_text_mode(1);
                }
                d->menu.game.mode = GAME_MENU_SELECT_SAVE;
            } else if(d->menu.game.save_idx == -1) {
                struct bork_save new_save;
                strncpy(new_save.name, d->menu.game.save_name, 32);
                ARR_PUSH(d->core->save_files, new_save);
                save_game(d, new_save.name);
                d->menu.game.mode = GAME_MENU_BASE;
            } else {
                struct bork_save* save = &d->core->save_files.data[d->menu.game.save_idx];
                save_game(d, save->name);
                d->menu.game.mode = GAME_MENU_BASE;
            }
        }
    /*  THE OPTIONS MENU    */
    } else if(d->menu.game.mode == GAME_MENU_SHOW_OPTIONS) {
        if(d->menu.game.remap_ctrl) {
            if(d->core->gpad_idx >= 0) {
                if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT))
                    d->menu.game.remap_ctrl = 0;
                uint8_t ctrl = pg_first_gamepad();
                if(ctrl) {
                    d->core->gpad_map[d->menu.game.opt_idx - BORK_OPTS] = ctrl;
                    d->menu.game.remap_ctrl = 0;
                }
            } else {
                uint8_t ctrl = pg_first_input();
                if(ctrl) {
                    d->core->ctrl_map[d->menu.game.opt_idx - BORK_OPTS] = ctrl;
                    d->menu.game.remap_ctrl = 0;
                }
            }
        } else if(d->menu.game.opt_res_typing) {
            if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
            || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
            || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
                d->menu.game.opt_res_typing = 0;
                pg_text_mode(0);
            } else if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
                   || pg_check_input(SDL_SCANCODE_KP_ENTER, PG_CONTROL_HIT)) {
                int res_opt = d->menu.game.opt_idx - 1;
                int input = (res_opt == 0) ? MAX(atoi(d->menu.game.opt_res_input), 160)
                                         : MAX(atoi(d->menu.game.opt_res_input), 90);
                d->menu.game.opt_res[res_opt] = input;
                d->menu.game.opt_res_typing = 0;
                d->menu.game.opt_res_input_idx = 0;
                pg_text_mode(0);
            } else if(d->menu.game.opt_res_input_idx > 0
                   && pg_check_input(SDL_SCANCODE_BACKSPACE, PG_CONTROL_HIT)) {
                d->menu.game.opt_res_input[--d->menu.game.opt_res_input_idx] = '\0';
            } else {
                int len = pg_copy_text_input(d->menu.game.opt_res_input + d->menu.game.opt_res_input_idx,
                                             5 - d->menu.game.opt_res_input_idx);
                d->menu.game.opt_res_input_idx += len;
            }
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            d->menu.game.mode = GAME_MENU_BASE;
            d->menu.game.opt_idx = 0;
            d->menu.game.opt_scroll = 0;
            if(d->menu.game.opt_fullscreen != d->core->fullscreen
            || d->menu.game.opt_res[0] != d->core->screen_size[0]
            || d->menu.game.opt_res[1] != d->core->screen_size[1]) {
                bork_reinit_gfx(d->core, d->menu.game.opt_res[0], d->menu.game.opt_res[1], d->menu.game.opt_fullscreen);
            }
            bork_save_options(d->core);
        } else if(click) {
            if(fabs(mouse_pos[0] - ar * 0.85) < 0.2
            && fabs(mouse_pos[1] - 0.115) < 0.03) {
                d->menu.game.mode = GAME_MENU_BASE;
            } else if(fabs(mouse_pos[0] - ar * 0.65) < 0.2
            && fabs(mouse_pos[1] - 0.115) < 0.03) {
                if(d->menu.game.opt_fullscreen != d->core->fullscreen
                || d->menu.game.opt_res[0] != d->core->screen_size[0]
                || d->menu.game.opt_res[1] != d->core->screen_size[1]) {
                    bork_reinit_gfx(d->core, d->menu.game.opt_res[0], d->menu.game.opt_res[1], d->menu.game.opt_fullscreen);
                }
                bork_save_options(d->core);
            } else {
                /*  Clicking on options */
                int i;
                for(i = 0; i < 10; ++i) {
                    if(fabs(mouse_pos[0] - (ar * 0.1 + 0.0125 + 0.25)) < 0.25
                    && fabs(mouse_pos[1] - (0.25 + i * 0.06 + 0.0125)) < 0.025) {
                        d->menu.game.opt_idx = d->menu.game.opt_scroll + i;
                    } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.1)) < 0.15
                           && fabs(mouse_pos[1] - (0.25 + i * 0.06 + 0.0125)) < 0.025) {
                        int opt = d->menu.game.opt_scroll + i;
                        d->menu.game.opt_idx = opt;
                        if(opt == BORK_CTRL_COUNT + BORK_OPTS) {
                            if(d->core->gpad_idx >= 0) {
                                bork_reset_gamepad_map(d->core);
                                d->core->joy_sensitivity = 1.0f;
                            } else {
                                bork_reset_keymap(d->core);
                                d->core->mouse_sensitivity = 1.0f / 1000;
                            }
                        } else if(opt >= BORK_OPTS) {
                            d->menu.game.remap_ctrl = 1;
                        } else {
                            int hit = 0;
                            switch(opt) {
                            case BORK_OPT_FULLSCREEN:
                                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
                                break;
                            case BORK_OPT_RES_X: case BORK_OPT_RES_Y:
                                d->menu.game.opt_res_typing = 1;
                                d->menu.game.opt_res_input_idx = 0;
                                pg_text_mode(1);
                                hit = 1;
                                break;
                            case BORK_OPT_SHOW_FPS:
                                d->core->show_fps = 1 - d->core->show_fps;
                                hit = 1;
                                break;
                            case BORK_OPT_GAMMA:
                                if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.035) {
                                    d->core->gamma = MAX(0.05, d->core->gamma - 0.05);
                                    hit = 1;
                                } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.25)) < 0.035) {
                                    d->core->gamma = MIN(5.0, d->core->gamma + 0.05);
                                    hit = 1;
                                }
                                bork_set_gamma(d->core, d->core->gamma);
                                break;
                            case BORK_OPT_MUSIC_VOL:
                                if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                    d->core->music_volume = MAX(0.0f, d->core->music_volume - 0.05);
                                    hit = 1;
                                } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.25)) < 0.035) {
                                    d->core->music_volume = MIN(2.0, d->core->music_volume + 0.05);
                                    hit = 1;
                                }
                                bork_set_music_volume(d->core, d->core->music_volume);
                                break;
                            case BORK_OPT_SFX_VOL:
                                if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                    d->core->sfx_volume = MAX(0.0f, d->core->sfx_volume - 0.05);
                                    hit = 1;
                                } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.25)) < 0.035) {
                                    d->core->sfx_volume = MIN(2.0, d->core->sfx_volume + 0.05);
                                    hit = 1;
                                }
                                bork_set_sfx_volume(d->core, d->core->sfx_volume);
                                break;
                            case BORK_OPT_INVERT_Y:
                                d->core->invert_y = 1 - d->core->invert_y;
                                hit = 1;
                                break;
                            case BORK_OPT_MOUSE_SENS:
                                if(d->core->gpad_idx >= 0) {
                                    if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                        d->core->joy_sensitivity =
                                            MAX(0.05, d->core->joy_sensitivity - 0.05);
                                        hit = 1;
                                    } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.2)) < 0.03) {
                                        d->core->joy_sensitivity =
                                            MIN(5.0f, d->core->joy_sensitivity + 0.05);
                                        hit = 1;
                                    }
                                } else {
                                    if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                        d->core->mouse_sensitivity =
                                            MAX(0.0001, d->core->mouse_sensitivity - 0.0001);
                                        hit = 1;
                                    } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.2)) < 0.03) {
                                        d->core->mouse_sensitivity =
                                            MIN(5.0f / 1000, d->core->mouse_sensitivity + 0.0001);
                                        hit = 1;
                                    }
                                }
                                break;
                            case BORK_OPT_GAMEPAD:
                                if(SDL_NumJoysticks() == 0) d->core->gpad_idx = -1;
                                else if(d->core->gpad_idx == SDL_NumJoysticks() - 1) d->core->gpad_idx = -1;
                                else ++d->core->gpad_idx;
                                pg_use_gamepad(d->core->gpad_idx);
                                hit = 1;
                                break;
                            }
                            if(hit) pg_audio_play(&d->core->menu_sound, 0.5);
                        }
                    }
                }
            }
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)
               || stick_ctrl_x == 1) {
            if(d->menu.game.opt_idx == BORK_OPT_FULLSCREEN) {
                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
            } else if(d->menu.game.opt_idx == BORK_OPT_SHOW_FPS) {
                d->core->show_fps = 1 - d->core->show_fps;
            } else if(d->menu.game.opt_idx == BORK_OPT_INVERT_Y) {
                d->core->invert_y = 1 - d->core->invert_y;
            } else if(d->menu.game.opt_idx == BORK_OPT_GAMMA) {
                d->core->gamma = MIN(5.0f, d->core->gamma + 0.05);
                bork_set_gamma(d->core, d->core->gamma);
            } else if(d->menu.game.opt_idx == BORK_OPT_MUSIC_VOL) {
                d->core->music_volume = MIN(2.0, d->core->music_volume + 0.05);
                bork_set_music_volume(d->core, d->core->music_volume);
            } else if(d->menu.game.opt_idx == BORK_OPT_SFX_VOL) {
                d->core->sfx_volume = MIN(2.0, d->core->sfx_volume + 0.05);
                bork_set_sfx_volume(d->core, d->core->sfx_volume);
            } else if(d->menu.game.opt_idx == BORK_OPT_MOUSE_SENS) {
                if(d->core->gpad_idx >= 0) {
                    d->core->joy_sensitivity = MIN(5.0f, d->core->joy_sensitivity + 0.05);
                } else {
                    d->core->mouse_sensitivity = MIN(5.0f / 1000, d->core->mouse_sensitivity + 0.0001);
                }
            } else if(d->menu.game.opt_idx == BORK_OPT_GAMEPAD) {
                int num_gpads = SDL_NumJoysticks();
                if(num_gpads == 0) d->core->gpad_idx = -1;
                else if(d->core->gpad_idx < num_gpads) ++d->core->gpad_idx;
                pg_use_gamepad(d->core->gpad_idx);
            }
        } else if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)
               || stick_ctrl_x == -1) {
            if(d->menu.game.opt_idx == BORK_OPT_FULLSCREEN) {
                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
            } else if(d->menu.game.opt_idx == BORK_OPT_SHOW_FPS) {
                d->core->show_fps = 1 - d->core->show_fps;
            } else if(d->menu.game.opt_idx == BORK_OPT_INVERT_Y) {
                d->core->invert_y = 1 - d->core->invert_y;
            } else if(d->menu.game.opt_idx == BORK_OPT_GAMMA) {
                d->core->gamma = MAX(0.05f, d->core->gamma - 0.05);
                bork_set_gamma(d->core, d->core->gamma);
            } else if(d->menu.game.opt_idx == BORK_OPT_MUSIC_VOL) {
                d->core->music_volume = MAX(0.0f, d->core->music_volume - 0.05);
                bork_set_music_volume(d->core, d->core->music_volume);
            } else if(d->menu.game.opt_idx == BORK_OPT_SFX_VOL) {
                d->core->sfx_volume = MAX(0.0f, d->core->sfx_volume - 0.05);
                bork_set_sfx_volume(d->core, d->core->sfx_volume);
            } else if(d->menu.game.opt_idx == BORK_OPT_MOUSE_SENS) {
                if(d->core->gpad_idx >= 0) {
                    d->core->joy_sensitivity = MAX(0.05, d->core->joy_sensitivity - 0.05);
                } else {
                    d->core->mouse_sensitivity = MAX(0.0001, d->core->mouse_sensitivity - 0.0001);
                }
            } else if(d->menu.game.opt_idx == BORK_OPT_GAMEPAD) {
                if(d->core->gpad_idx > -1) {
                    --d->core->gpad_idx;
                    pg_use_gamepad(d->core->gpad_idx);
                }
            }
        } else if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            if(d->menu.game.opt_idx == BORK_OPT_FULLSCREEN) {
                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
            } else if(d->menu.game.opt_idx == BORK_OPT_RES_X || d->menu.game.opt_idx == BORK_OPT_RES_Y) {
                d->menu.game.opt_res_typing = 1;
                pg_text_mode(1);
            } else if(d->menu.game.opt_idx == BORK_OPT_SHOW_FPS) {
                d->core->show_fps = 1 - d->core->show_fps;
            } else if(d->menu.game.opt_idx == BORK_OPT_INVERT_Y) {
                d->core->invert_y = 1 - d->core->invert_y;
            } else if(d->menu.game.opt_idx == BORK_CTRL_COUNT + BORK_OPTS) {
                if(d->core->gpad_idx >= 0) {
                    bork_reset_gamepad_map(d->core);
                    d->core->joy_sensitivity = 1.0f;
                } else {
                    bork_reset_keymap(d->core);
                    d->core->mouse_sensitivity = 1.0f / 1000;
                }
            } else if(d->menu.game.opt_idx >= BORK_OPTS) d->menu.game.remap_ctrl = 1;
        } else if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
               || stick_ctrl_y == 1) {
            d->menu.game.opt_idx = MIN(BORK_CTRL_COUNT + BORK_OPTS, d->menu.game.opt_idx + 1);
            if(d->menu.game.opt_scroll + 9 < d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx - 9;
            else if(d->menu.game.opt_scroll > d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx;
            pg_audio_play(&d->core->menu_sound, 0.5);
        } else if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
               || stick_ctrl_y == -1) {
            d->menu.game.opt_idx = MAX(0, d->menu.game.opt_idx - 1);
            if(d->menu.game.opt_scroll + 9 < d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx - 9;
            else if(d->menu.game.opt_scroll > d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx;
            pg_audio_play(&d->core->menu_sound, 0.5);
        } else if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)
        && d->menu.game.opt_scroll > 0) {
            d->menu.game.opt_scroll -= 1;
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
        && d->menu.game.opt_scroll + 10 < BORK_FULL_OPTS) {
            d->menu.game.opt_scroll += 1;
        }
    }



    /*
    } else if(d->menu.game.mode == GAME_MENU_SHOW_OPTIONS) {
        if(d->menu.game.remap_ctrl) {
            uint8_t ctrl = pg_first_input();
            if(ctrl) {
                d->core->ctrl_map[d->menu.game.opt_idx - BORK_OPTS] = ctrl;
                d->menu.game.remap_ctrl = 0;
            }
        } else if(d->menu.game.opt_res_typing) {
            if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
                d->menu.game.opt_res_typing = 0;
                pg_text_mode(0);
            } else if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)) {
                int res_opt = d->menu.game.opt_idx - 1;
                int input = atoi(d->menu.game.opt_res_input);
                int min_res = res_opt == 0 ? 160 : 90;
                d->menu.game.opt_res[res_opt] = MAX(input, min_res);
                d->menu.game.opt_res_typing = 0;
                d->menu.game.opt_res_input_idx = 0;
                pg_text_mode(0);
            } else if(d->menu.game.opt_res_input_idx > 0
                   && pg_check_input(SDL_SCANCODE_BACKSPACE, PG_CONTROL_HIT)) {
                d->menu.game.opt_res_input[--d->menu.game.opt_res_input_idx] = '\0';
            } else {
                int len = pg_copy_text_input(d->menu.game.opt_res_input + d->menu.game.opt_res_input_idx,
                                             5 - d->menu.game.opt_res_input_idx);
                d->menu.game.opt_res_input_idx += len;
            }
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
               || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            d->menu.game.mode = GAME_MENU_BASE;
            d->menu.game.opt_idx = 0;
            d->menu.game.opt_scroll = 0;
            if(d->menu.game.opt_fullscreen != d->core->fullscreen
            || d->menu.game.opt_res[0] != d->core->screen_size[0]
            || d->menu.game.opt_res[1] != d->core->screen_size[1]) {
                bork_reinit_gfx(d->core, d->menu.game.opt_res[0], d->menu.game.opt_res[1], d->menu.game.opt_fullscreen);
            }
            bork_save_options(d->core);
        } else if(click) {
            if(fabs(mouse_pos[0] - ar * 0.85) < 0.2
            && fabs(mouse_pos[1] - 0.115) < 0.03) {
                d->menu.game.mode = GAME_MENU_BASE;
            } else if(fabs(mouse_pos[0] - ar * 0.65) < 0.2
            && fabs(mouse_pos[1] - 0.115) < 0.03) {
                if(d->menu.game.opt_fullscreen != d->core->fullscreen
                || d->menu.game.opt_res[0] != d->core->screen_size[0]
                || d->menu.game.opt_res[1] != d->core->screen_size[1]) {
                    bork_reinit_gfx(d->core, d->menu.game.opt_res[0], d->menu.game.opt_res[1], d->menu.game.opt_fullscreen);
                }
                bork_save_options(d->core);
            } else {
                int i;
                for(i = 0; i < 10; ++i) {
                    if(fabs(mouse_pos[0] - (ar * 0.1 + 0.0125 + 0.25)) < 0.25
                    && fabs(mouse_pos[1] - (0.25 + i * 0.06 + 0.0125)) < 0.025) {
                        d->menu.game.opt_idx = d->menu.game.opt_scroll + i;
                    } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.1)) < 0.15
                           && fabs(mouse_pos[1] - (0.25 + i * 0.06 + 0.0125)) < 0.025) {
                        int opt = d->menu.game.opt_scroll + i;
                        d->menu.game.opt_idx = opt;
                        if(opt == 0) d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
                        else if(opt == 1 || opt == 2) {
                            d->menu.game.opt_res_typing = 1;
                            d->menu.game.opt_res_input_idx = 0;
                            pg_text_mode(1);
                        } else if(opt == 3) {
                            d->core->show_fps = 1 - d->core->show_fps;
                        } else if(opt == 4) {
                            d->core->invert_y = 1 - d->core->invert_y;
                        } else if(opt == 5) {
                            if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                d->core->mouse_sensitivity =
                                    MAX(0.0001, d->core->mouse_sensitivity - 0.0001);
                            } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.2)) < 0.03) {
                                d->core->mouse_sensitivity =
                                    MIN(5.0f / 1000, d->core->mouse_sensitivity + 0.0001);
                            }
                        } else if(opt == 6) {
                            if(fabs(mouse_pos[0] - (ar * 0.65 + 0.0125)) < 0.03) {
                                d->core->gamma = MAX(0.05, d->core->gamma - 0.05);
                            } else if(fabs(mouse_pos[0] - (ar * 0.65 + 0.2)) < 0.03) {
                                d->core->gamma = MIN(5.0, d->core->gamma + 0.05);
                            }
                            pg_postproc_gamma_set(&d->core->post_gamma, d->core->gamma);
                        } else if(opt == 7) {
                            if(SDL_NumJoysticks() == 0) d->core->gpad_idx = -1;
                            else if(d->core->gpad_idx == SDL_NumJoysticks() - 1) d->core->gpad_idx = -1;
                            else ++d->core->gpad_idx;
                            pg_use_gamepad(d->core->gpad_idx);
                        } else if(opt == BORK_CTRL_COUNT + BORK_OPTS) {
                            bork_reset_keymap(d->core);
                            d->core->mouse_sensitivity = 1.0f / 1000;
                        } else if(opt >= BORK_OPTS) {
                            d->menu.game.remap_ctrl = 1;
                        }
                    }
                }
            }
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)) {
            if(d->menu.game.opt_idx == 0) {
                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
            } else if(d->menu.game.opt_idx == 3) {
                d->core->show_fps = 1 - d->core->show_fps;
            } else if(d->menu.game.opt_idx == 4) {
                d->core->mouse_sensitivity = MIN(5.0f / 1000, d->core->mouse_sensitivity + 0.0001);
            } else if(d->menu.game.opt_idx == 7) {
                int num_gpads = SDL_NumJoysticks();
                if(d->core->gpad_idx < num_gpads) {
                    ++d->core->gpad_idx;
                    pg_use_gamepad(d->core->gpad_idx);
                }
            }
        } else if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
               || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)) {
            if(d->menu.game.opt_idx == 0) {
                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
            } else if(d->menu.game.opt_idx == 3) {
                d->core->show_fps = 1 - d->core->show_fps;
            } else if(d->menu.game.opt_idx == 4) {
                d->core->mouse_sensitivity = MAX(0.0001, d->core->mouse_sensitivity - 0.0001);
            } else if(d->menu.game.opt_idx == 7) {
                if(d->core->gpad_idx > -1) {
                    --d->core->gpad_idx;
                    pg_use_gamepad(d->core->gpad_idx);
                }
            }
        } else if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            if(d->menu.game.opt_idx == 1 || d->menu.game.opt_idx == 2) {
                d->menu.game.opt_res_typing = 1;
                pg_text_mode(1);
            } else if(d->menu.game.opt_idx == 0) {
                d->menu.game.opt_fullscreen = 1 - d->menu.game.opt_fullscreen;
            } else if(d->menu.game.opt_idx == 3) {
                d->core->show_fps = 1 - d->core->show_fps;
            } else if(d->menu.game.opt_idx == 4) {
                d->core->invert_y = 1 - d->core->invert_y;
            } else if(d->menu.game.opt_idx == BORK_CTRL_COUNT + BORK_OPTS) {
                bork_reset_keymap(d->core);
                d->core->mouse_sensitivity = 1.0f / 1000;
            } else if(d->menu.game.opt_idx > BORK_OPTS) d->menu.game.remap_ctrl = 1;
        } else if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
               || stick_ctrl_y == 1) {
            d->menu.game.opt_idx = MIN(BORK_CTRL_COUNT + BORK_OPTS, d->menu.game.opt_idx + 1);
            if(d->menu.game.opt_scroll + 9 < d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx - 9;
            else if(d->menu.game.opt_scroll > d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx;
            pg_audio_play(&d->core->menu_sound, 0.2);
        } else if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
               || pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
               || stick_ctrl_y == -1) {
            d->menu.game.opt_idx = MAX(0, d->menu.game.opt_idx - 1);
            if(d->menu.game.opt_scroll + 9 < d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx - 9;
            else if(d->menu.game.opt_scroll > d->menu.game.opt_idx) d->menu.game.opt_scroll = d->menu.game.opt_idx;
            pg_audio_play(&d->core->menu_sound, 0.2);
        } else if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)
        && d->menu.game.opt_scroll > 0) {
            d->menu.game.opt_scroll -= 1;
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
        && d->menu.game.opt_scroll + 10 < BORK_CTRL_COUNT + BORK_OPTS + 1) {
            d->menu.game.opt_scroll += 1;
        }
    }*/
}

void draw_game_menu(struct bork_play_data* d, float t)
{
    static char* game_menu_opts[6] = {
        "RESUME",
        "SAVE GAME",
        "LOAD GAME",
        "OPTIONS",
        "EXIT TO MENU",
        "QUIT GAME" };
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = {
        .use_blocks = 1,
        .block = { "PAWS MENU" },
        .block_style = { { 0.1, 0.1, 0.05, 1.25 } },
        .block_color = { { 1, 1, 1, 0.7 } } };
    int i;
    int ti = 1;
    float base_menu_color = (d->menu.game.mode == GAME_MENU_BASE ? 1 : 0.25);
    for(i = 0; i < 6; ++i) {
        int is_selected = (i == d->menu.game.selection_idx);
        snprintf(text.block[++ti], 16, "%s", game_menu_opts[i]);
        vec4_set(text.block_style[ti], 0.15 + 0.05 * is_selected,
                 0.25 + 0.1 * i, 0.04, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, is_selected ? 1 : base_menu_color);
    }
    if(d->menu.game.mode == GAME_MENU_LOAD) {
        int num_saves = MIN(d->core->save_files.len, 6);
        for(i = 0; i < num_saves; ++i) {
            int is_selected = (i + d->menu.game.save_scroll == d->menu.game.save_idx);
            struct bork_save* save = &d->core->save_files.data[i + d->menu.game.save_scroll];
            snprintf(text.block[++ti], 32, "%s", save->name);
            vec4_set(text.block_style[ti], ar * 0.55 + 0.04 * is_selected,
                     0.25 + 0.1 * i, 0.03, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        }
        int len = snprintf(text.block[++ti], 32, "SELECT SAVE FILE");
        vec4_set(text.block_style[ti], ar * 0.5 - len * 0.05 * 1.2 * 0.5,
                 0.85, 0.05, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
    } else if(d->menu.game.mode == GAME_MENU_SELECT_SAVE
           || d->menu.game.mode == GAME_MENU_EDIT_SAVE
           || d->menu.game.mode == GAME_MENU_CONFIRM_OVERWRITE) {
        int num_saves = MIN(d->core->save_files.len + 1, 6);
        for(i = 0; i < num_saves; ++i) {
            int is_selected = (i + d->menu.game.save_scroll == d->menu.game.save_idx);
            if(i + d->menu.game.save_scroll == -1) {
                snprintf(text.block[++ti], 32, "%s",
                                   d->menu.game.save_name);
                vec4_set(text.block_style[ti], ar * 0.55 + 0.04 * is_selected,
                         0.25, 0.04, 1.2);
                vec4_set(text.block_color[ti], 1, 1, 1, 1);
                if(d->menu.game.mode == GAME_MENU_EDIT_SAVE) {
                    snprintf(text.block[++ti], 2, ">");
                    vec4_set(text.block_style[ti], ar * 0.55 - 0.04, 0.25, 0.04, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                }
                continue;
            }
            struct bork_save* save = &d->core->save_files.data[i + d->menu.game.save_scroll];
            snprintf(text.block[++ti], 32, "%s", save->name);
            vec4_set(text.block_style[ti], ar * 0.55 + 0.04 * is_selected,
                     0.25 + 0.1 * i, 0.03, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1,
                     (d->menu.game.mode == GAME_MENU_EDIT_SAVE ? 0.25 : 1));
        }
        if(d->menu.game.mode == GAME_MENU_CONFIRM_OVERWRITE) {
            int len = snprintf(text.block[++ti], 32, "CONFIRM OVERWRITE");
            vec4_set(text.block_style[ti], ar * 0.5 - len * 0.05 * 1.2 * 0.5,
                     0.8, 0.05, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            len = snprintf(text.block[++ti], 16, "YES");
            vec4_set(text.block_style[ti], ar * 0.45 - (len * 0.03 * 1.25 * 0.5), 0.89, 0.03, 1.25);
            if(d->menu.game.confirm_opt)
                vec4_set(text.block_color[ti], 0.2, 0.2, 0.2, 0.9);
            else vec4_set(text.block_color[ti], 1, 1, 1, 1);
            len = snprintf(text.block[++ti], 16, "NO");
            vec4_set(text.block_style[ti], ar * 0.55 - (len * 0.03 * 1.25 * 0.5), 0.89, 0.03, 1.25);
            if(!d->menu.game.confirm_opt)
                vec4_set(text.block_color[ti], 0.2, 0.2, 0.2, 0.9);
            else vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else {
            int len = snprintf(text.block[++ti], 32, "SELECT SAVE FILE");
            vec4_set(text.block_style[ti], ar * 0.5 - len * 0.05 * 1.2 * 0.5,
                     0.85, 0.05, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        }
    } else if(d->menu.game.mode == GAME_MENU_SHOW_OPTIONS) {
        text = (struct pg_shader_text){};
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
            int opt = d->menu.game.opt_scroll + i;
            int is_selected = (d->menu.game.opt_scroll + i == d->menu.game.opt_idx);
            if(opt == BORK_OPT_FULLSCREEN) {
                strncpy(text.block[++ti], "FULLSCREEN", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, d->menu.game.opt_fullscreen ? "YES" : "NO");
            } else if(opt == BORK_OPT_RES_X) {
                strncpy(text.block[++ti], "RES. WIDTH", 32);
                memset(text.block[ti + 1], 0, 32);
                if(d->menu.game.opt_res_typing && d->menu.game.opt_idx == opt) {
                    snprintf(text.block[ti + 1], 32, ">%s", d->menu.game.opt_res_input);
                } else {
                    snprintf(text.block[ti + 1], 32, "%d", d->menu.game.opt_res[0]);
                }
            } else if(opt == BORK_OPT_RES_Y) {
                strncpy(text.block[++ti], "RES. HEIGHT", 32);
                memset(text.block[ti + 1], 0, 32);
                if(d->menu.game.opt_res_typing && d->menu.game.opt_idx == opt) {
                    snprintf(text.block[ti + 1], 32, ">%s", d->menu.game.opt_res_input);
                } else {
                    snprintf(text.block[ti + 1], 32, "%d", d->menu.game.opt_res[1]);
                }
            } else if(opt == BORK_OPT_SHOW_FPS) {
                strncpy(text.block[++ti], "SHOW FPS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "%s", d->core->show_fps ? "YES" : "NO");
            } else if(opt == BORK_OPT_GAMMA) {
                strncpy(text.block[++ti], "GAMMA CORRECTION", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "< %.2f >", d->core->gamma);
            } else if(opt == BORK_OPT_MUSIC_VOL) {
                strncpy(text.block[++ti], "MUSIC VOLUME", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "< %.2f >", d->core->music_volume);
            } else if(opt == BORK_OPT_SFX_VOL) {
                strncpy(text.block[++ti], "SFX VOLUME", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "< %.2f >", d->core->sfx_volume);
            } else if(opt == BORK_OPT_INVERT_Y) {
                strncpy(text.block[++ti], "INVERT Y AXIS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "%s", d->core->invert_y ? "YES" : "NO");
            } else if(opt == BORK_OPT_MOUSE_SENS) {
                if(d->core->gpad_idx >= 0) {
                    strncpy(text.block[++ti], "JOYSTICK SENSITIVITY", 32);
                    memset(text.block[ti + 1], 0, 32);
                    snprintf(text.block[ti + 1], 32, "< %.2f >", d->core->joy_sensitivity);
                } else {
                    strncpy(text.block[++ti], "MOUSE SENSITIVITY", 32);
                    memset(text.block[ti + 1], 0, 32);
                    snprintf(text.block[ti + 1], 32, "< %.1f >", d->core->mouse_sensitivity * 1000);
                }
            } else if(opt == BORK_OPT_GAMEPAD) {
                strncpy(text.block[++ti], "SELECT GAMEPAD", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "%s",
                    d->core->gpad_idx == -1 ? "DISABLED" :
                        SDL_GameControllerNameForIndex(d->core->gpad_idx));
            } else if(opt == BORK_CTRL_COUNT + BORK_OPTS) {
                strncpy(text.block[++ti], "RESET TO DEFAULTS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "[RESET]");
            } else {
                strncpy(text.block[++ti], bork_get_ctrl_name(opt - BORK_OPTS), 32);
                if(d->core->gpad_idx >= 0) {
                    if(opt - BORK_OPTS < BORK_CTRL_JUMP) {
                        strncpy(text.block[ti + 1], "---", 32);
                    } else {
                        strncpy(text.block[ti + 1],
                            pg_gamepad_name(d->core->gpad_map[opt - BORK_OPTS]), 32);
                    }
                } else {
                    strncpy(text.block[ti + 1],
                        pg_input_name(d->core->ctrl_map[opt - BORK_OPTS]), 32);
                }
            }
            vec4_set(text.block_style[ti], ar * 0.1 + is_selected * 0.05,
                                           0.25 + i * 0.06, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            vec4_set(text.block_style[ti + 1], ar * 0.65, 0.25 + i * 0.06, 0.025, 1.2);
            vec4_set(text.block_color[ti + 1], 1, 1, 1, 1);
            ti += 1;
        }
        if(d->menu.game.remap_ctrl) {
            len = snprintf(text.block[++ti], 64, "PRESS DESIRED CONTROL FOR");
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            len = snprintf(text.block[++ti], 64, "%s", bork_get_ctrl_name(d->menu.game.opt_idx - BORK_OPTS));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.925, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else {
            switch(d->menu.game.opt_idx) {
                case BORK_OPTS + BORK_CTRL_COUNT:
                    len = snprintf(text.block[++ti], 64, "!!! PRESS %s TO RESET CONTROLS !!!",
                                   pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
                case BORK_OPT_FULLSCREEN:
                    len = snprintf(text.block[++ti], 64, "PRESS %s TO TOGGLE FULLSCREEN",
                                   pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
                case BORK_OPT_RES_X: case BORK_OPT_RES_Y:
                    len = snprintf(text.block[++ti], 64, "PRESS %s TO SET RESOLUTION",
                                   pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
                case BORK_OPT_SHOW_FPS:
                    len = snprintf(text.block[++ti], 64, "PRESS %s TO TOGGLE FPS DISPLAY",
                                   pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
                case BORK_OPT_INVERT_Y:
                    len = snprintf(text.block[++ti], 64, "PRESS %s TO INVERT Y AXIS",
                                   pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
                case BORK_OPT_GAMMA: case BORK_OPT_MOUSE_SENS: case BORK_OPT_GAMEPAD:
                case BORK_OPT_MUSIC_VOL: case BORK_OPT_SFX_VOL:
                    len = snprintf(text.block[++ti], 64, "LEFT/RIGHT TO SET OPTION");
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
                default:
                    len = snprintf(text.block[++ti], 64, "PRESS %s TO RE-MAP CONTROL",
                                   pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
                    vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
                    vec4_set(text.block_color[ti], 1, 1, 1, 1);
                    break;
            }
        }
        text.use_blocks = ti + 1;
    }

/*
    } else if(d->menu.game.mode == GAME_MENU_SHOW_OPTIONS) {
        text = (struct pg_shader_text){};
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
            int opt = d->menu.game.opt_scroll + i;
            int is_selected = (d->menu.game.opt_scroll + i == d->menu.game.opt_idx);
            if(opt == 0) {
                strncpy(text.block[++ti], "FULLSCREEN", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, d->menu.game.opt_fullscreen ? "YES" : "NO");
            } else if(opt == 1) {
                strncpy(text.block[++ti], "RES. WIDTH", 32);
                memset(text.block[ti + 1], 0, 32);
                if(d->menu.game.opt_res_typing && d->menu.game.opt_idx == opt) {
                    snprintf(text.block[ti + 1], 32, ">%s", d->menu.game.opt_res_input);
                } else {
                    snprintf(text.block[ti + 1], 32, "%d", d->menu.game.opt_res[0]);
                }
            } else if(opt == 2) {
                strncpy(text.block[++ti], "RES. HEIGHT", 32);
                memset(text.block[ti + 1], 0, 32);
                if(d->menu.game.opt_res_typing && d->menu.game.opt_idx == opt) {
                    snprintf(text.block[ti + 1], 32, ">%s", d->menu.game.opt_res_input);
                } else {
                    snprintf(text.block[ti + 1], 32, "%d", d->menu.game.opt_res[1]);
                }
            } else if(opt == 3) {
                strncpy(text.block[++ti], "SHOW FPS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "%s", d->core->show_fps ? "YES" : "NO");
            } else if(opt == 4) {
                strncpy(text.block[++ti], "INVERT Y AXIS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "%s", d->core->invert_y ? "YES" : "NO");
            } else if(opt == 5) {
                strncpy(text.block[++ti], "MOUSE SENSITIVITY", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "< %.1f >", d->core->mouse_sensitivity * 1000);
            } else if(opt == 6) {
                strncpy(text.block[++ti], "GAMMA CORRECTION", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "< %.2f >", d->core->gamma);
            } else if(opt == 7) {
                strncpy(text.block[++ti], "SELECT GAMEPAD", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "%s",
                    d->core->gpad_idx == -1 ? "DISABLED" :
                        SDL_GameControllerNameForIndex(d->core->gpad_idx));
            } else if(opt == BORK_CTRL_COUNT + BORK_OPTS) {
                strncpy(text.block[++ti], "RESET TO DEFAULTS", 32);
                memset(text.block[ti + 1], 0, 32);
                snprintf(text.block[ti + 1], 32, "[RESET]");
            } else {
                strncpy(text.block[++ti], bork_get_ctrl_name(opt - BORK_OPTS), 32);
                strncpy(text.block[ti + 1], pg_input_name(d->core->ctrl_map[opt - BORK_OPTS]), 32);
            }
            vec4_set(text.block_style[ti], ar * 0.1 + is_selected * 0.05,
                                           0.25 + i * 0.06, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            vec4_set(text.block_style[ti + 1], ar * 0.65, 0.25 + i * 0.06, 0.025, 1.2);
            vec4_set(text.block_color[ti + 1], 1, 1, 1, 1);
            ti += 1;
        }
        if(d->menu.game.remap_ctrl) {
            len = snprintf(text.block[++ti], 64, "PRESS DESIRED CONTROL FOR");
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
            len = snprintf(text.block[++ti], 64, "%s", bork_get_ctrl_name(d->menu.game.opt_idx - BORK_OPTS));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.925, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->menu.game.opt_idx == BORK_OPTS + BORK_CTRL_COUNT) {
            len = snprintf(text.block[++ti], 64, "!!! PRESS %s TO RESET CONTROLS !!!",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->menu.game.opt_idx >= BORK_OPTS) {
            len = snprintf(text.block[++ti], 64, "PRESS %s TO RE-MAP CONTROL",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->menu.game.opt_idx == 0) {
            len = snprintf(text.block[++ti], 64, "PRESS %s TO TOGGLE FULLSCREEN",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->menu.game.opt_idx == 1 || d->menu.game.opt_idx == 2) {
            len = snprintf(text.block[++ti], 64, "PRESS %s TO SET RESOLUTION",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->menu.game.opt_idx == 3) {
            len = snprintf(text.block[++ti], 64, "PRESS %s TO TOGGLE FPS DISPLAY",
                           pg_input_name(d->core->ctrl_map[BORK_CTRL_SELECT]));
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        } else if(d->menu.game.opt_idx == 4 || d->menu.game.opt_idx == 5 || d->menu.game.opt_idx == 6 || d->menu.game.opt_idx == 7) {
            len = snprintf(text.block[++ti], 64, "LEFT/RIGHT TO SET OPTION");
            vec4_set(text.block_style[ti], ar * 0.5 - (len * 0.025 * 1.2 * 0.5), 0.875, 0.025, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, 1);
        }
        text.use_blocks = ti + 1;
    }*/
    pg_shader_text_write(shader, &text);
    text.use_blocks = ti + 1;
    pg_shader_text_write(shader, &text);
}


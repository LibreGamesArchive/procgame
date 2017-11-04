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
    float opt_size[5] = { 0.5, 0.7, 0.7, 0.9, 0.7 };
    for(i = 0; i < 5; ++i) {
        if(mouse_pos[0] > 0.1 && mouse_pos[0] < opt_size[i]
        && mouse_pos[1] > (0.25 + 0.1 * i) && mouse_pos[1] < (0.25 + 0.1 * i + 0.05)) {
            if(d->menu.game.mode == GAME_MENU_BASE) {
                d->menu.game.selection_idx = i;
                mouse_over_option = 1;
            } else if(click) {
                d->menu.game.mode = GAME_MENU_BASE;
            }
        }
    }
    if(d->menu.game.mode == GAME_MENU_SELECT_SAVE
    || d->menu.game.mode == GAME_MENU_LOAD) {
        int num_saves = MIN(d->core->save_files.len + 1, 6);
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
        if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
            d->menu.state = BORK_MENU_CLOSED;
            SDL_ShowCursor(SDL_DISABLE);
            pg_mouse_mode(1);
        }
        if(pg_check_input(SDL_SCANCODE_S, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            d->menu.game.selection_idx = MIN(d->menu.game.selection_idx + 1, 4);
        } else if(pg_check_input(SDL_SCANCODE_W, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            d->menu.game.selection_idx = MAX(d->menu.game.selection_idx - 1, 0);
        }
        if((click && mouse_over_option)
        || pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
            switch(d->menu.game.selection_idx) {
                case 0:
                    d->menu.state = BORK_MENU_CLOSED;
                    pg_mouse_mode(1);
                    SDL_ShowCursor(SDL_DISABLE);
                    break;
                case 1: {
                    d->menu.game.mode = GAME_MENU_SELECT_SAVE;
                    d->menu.game.save_scroll = -1;
                    d->menu.game.save_idx = -1;
                    d->menu.game.save_name_len = 0;
                    snprintf(d->menu.game.save_name, 32, "NEW SAVE");
                    break;
                } case 2: {
                    d->menu.game.mode = GAME_MENU_LOAD;
                    d->menu.game.save_scroll = 0;
                    d->menu.game.save_idx = 0;
                    break;
                } case 3: {
                    struct bork_game_core* core = d->core;
                    bork_play_deinit(d);
                    bork_menu_start(state, core);
                    pg_flush_input();
                    return;
                } case 4: state->running = 0; break;
            }
        }
    } else if(d->menu.game.mode == GAME_MENU_LOAD) {
        if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
            d->menu.game.mode = GAME_MENU_BASE;
            d->menu.game.save_idx = 0;
        }
        if(pg_check_input(SDL_SCANCODE_S, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            d->menu.game.save_idx = MIN(d->menu.game.save_idx + 1, d->core->save_files.len - 1);
            if(d->menu.game.save_idx >= d->menu.game.save_scroll + 6) {
                ++d->menu.game.save_scroll;
            }
        } else if(pg_check_input(SDL_SCANCODE_W, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            d->menu.game.save_idx = MAX(d->menu.game.save_idx - 1, 0);
            if(d->menu.game.save_idx < d->menu.game.save_scroll) {
                --d->menu.game.save_scroll;
            }
        }
        if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
            struct bork_save* save = &d->core->save_files.data[d->menu.game.save_idx];
            load_game(d, save->name);
            d->menu.game.mode = GAME_MENU_BASE;
        }
    } else if(d->menu.game.mode == GAME_MENU_SELECT_SAVE) {
        if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            d->menu.game.mode = GAME_MENU_BASE;
            d->menu.game.save_idx = 0;
            d->menu.game.save_name_len = 0;
            d->menu.game.save_scroll = 0;
        }
        if(pg_check_input(SDL_SCANCODE_S, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)
        || stick_ctrl_y == -1) {
            d->menu.game.save_idx = MIN(d->menu.game.save_idx + 1, d->core->save_files.len - 1);
            if(d->menu.game.save_idx >= d->menu.game.save_scroll + 6) {
                ++d->menu.game.save_scroll;
            }
        } else if(pg_check_input(SDL_SCANCODE_W, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)
        || stick_ctrl_y == 1) {
            d->menu.game.save_idx = MAX(d->menu.game.save_idx - 1, -1);
            if(d->menu.game.save_idx < d->menu.game.save_scroll) {
                --d->menu.game.save_scroll;
            }
        }
        if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)) {
            if(d->menu.game.save_idx == -1) {
                d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                pg_text_mode(1);
            } else {
                d->menu.game.mode = GAME_MENU_CONFIRM_OVERWRITE;
            }
        } else if(pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
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
        d->menu.game.save_name_len += len;
        if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            d->menu.game.mode = GAME_MENU_SELECT_SAVE;
            pg_text_mode(0);
        } else if(pg_check_keycode(SDLK_BACKSPACE, PG_CONTROL_HIT)) {
            if(d->menu.game.save_name_len > 0) {
                d->menu.game.save_name[--d->menu.game.save_name_len] = '\0';
            }
        } else if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
               || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
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
            printf("num_saves: %d\n", (int)d->core->save_files.len);
            save_game(d, new_save.name);
            d->menu.game.save_idx = 0;
            d->menu.game.save_scroll = 0;
            d->menu.game.mode = GAME_MENU_BASE;
        }
    } else if(d->menu.game.mode == GAME_MENU_CONFIRM_OVERWRITE) {
        if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
            if(d->menu.game.save_idx == -1) {
                d->menu.game.mode = GAME_MENU_EDIT_SAVE;
                pg_text_mode(1);
            }
            d->menu.game.mode = GAME_MENU_SELECT_SAVE;
        }
        if(pg_check_input(SDL_SCANCODE_A, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HIT)
        || stick_ctrl_x == -1) {
            d->menu.game.confirm_opt = 0;
        } else if(pg_check_input(SDL_SCANCODE_D, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HIT)
        || stick_ctrl_x == 1) {
            d->menu.game.confirm_opt = 1;
        } 
        if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)
        || pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
        || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
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
    }
}

void draw_game_menu(struct bork_play_data* d, float t)
{
    static char* game_menu_opts[5] = {
        "RESUME",
        "SAVE GAME",
        "LOAD GAME",
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
    for(i = 0; i < 5; ++i) {
        int is_selected = (i == d->menu.game.selection_idx);
        int len = snprintf(text.block[++ti], 16, "%s", game_menu_opts[i]);
        vec4_set(text.block_style[ti], 0.15 + 0.05 * is_selected,
                 0.25 + 0.1 * i, 0.04, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, is_selected ? 1 : base_menu_color);
    }
    if(d->menu.game.mode == GAME_MENU_LOAD) {
        int num_saves = MIN(d->core->save_files.len, 6);
        for(i = 0; i < num_saves; ++i) {
            int is_selected = (i + d->menu.game.save_scroll == d->menu.game.save_idx);
            struct bork_save* save = &d->core->save_files.data[i + d->menu.game.save_scroll];
            int len = snprintf(text.block[++ti], 32, "%s", save->name);
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
                int len = snprintf(text.block[++ti], 32, "%s",
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
            int len = snprintf(text.block[++ti], 32, "%s", save->name);
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
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(shader, &text);
}


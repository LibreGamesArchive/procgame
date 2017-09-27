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
#include "datapad_content.h"

void tick_doorpad(struct bork_play_data* d)
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

void draw_doorpad(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1.0f });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
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


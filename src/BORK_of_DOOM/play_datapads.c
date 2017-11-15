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

void tick_datapad_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_CLOSED;
        SDL_ShowCursor(SDL_DISABLE);
        pg_mouse_mode(1);
        pg_audio_channel_pause(1, 0);
    }
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    int dp_idx = d->held_datapads[d->menu.datapads.selection_idx];
    const struct bork_datapad* dp = &BORK_DATAPADS[dp_idx];
    if(d->menu.datapads.side == 0) {
        int sel = d->menu.datapads.selection_idx;
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1) {
            d->menu.datapads.selection_idx =
                MIN(d->menu.datapads.selection_idx + 1, d->num_held_datapads);
            if(d->menu.datapads.selection_idx >= d->menu.recycler.scroll_idx + 10)
                ++d->menu.datapads.scroll_idx;
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
            d->menu.datapads.selection_idx = MAX(d->menu.datapads.selection_idx - 1, 0);
            if(d->menu.datapads.selection_idx < d->menu.datapads.scroll_idx)
                --d->menu.datapads.scroll_idx;
        }
        if(sel != d->menu.datapads.selection_idx) d->menu.datapads.text_scroll = 0;
    } else if(d->menu.datapads.side == 1 && dp->lines > 8) {
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1) {
            d->menu.datapads.selection_idx = MIN(d->menu.datapads.text_scroll + 1, dp->lines - 8);
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
            d->menu.datapads.selection_idx = MAX(d->menu.datapads.text_scroll - 1, 0);
        }
    }
}

void draw_datapad_menu(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = {
        .use_blocks = 1,
        .block = { "DATAPADS" },
        .block_style = { { 0.1, 0.1, 0.05, 1.25 } },
        .block_color = { { 1, 1, 1, 0.7 } } };
    int list_len = MIN(10, d->num_held_datapads);
    int i;
    int ti = 1;
    for(i = 0; i < list_len; ++i) {
        int dp_idx = d->held_datapads[d->menu.datapads.scroll_idx + i];
        const struct bork_datapad* dp = &BORK_DATAPADS[dp_idx];
        int is_selected = (i + d->menu.datapads.scroll_idx == d->menu.datapads.selection_idx);
        strncpy(text.block[++ti], dp->title, 64);
        vec4_set(text.block_style[ti], 0.1 + 0.05 * is_selected, 0.2 + 0.06 * i, 0.03, 1.2);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
    }
    int dp_idx = d->held_datapads[d->menu.datapads.selection_idx];
    const struct bork_datapad* dp = &BORK_DATAPADS[dp_idx];
    int text_len = MIN(8, dp->lines);
    int text_start = d->menu.datapads.text_scroll;
    if(d->num_held_datapads) {
        for(i = 0; i < text_len; ++i) {
            int len;
            len = snprintf(text.block[++ti], 32, "%s", dp->text[text_start + i]);
            vec4_set(text.block_style[ti], ar * 0.5, 0.3 + i * 0.03, 0.02, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, d->menu.datapads.side ? 0.75 : 1);
        }
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(shader, &text);
    shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    if(d->num_held_datapads) {
        if(text_start > 0) {
            pg_shader_2d_tex_frame(shader, 198);
            pg_shader_2d_transform(shader, (vec2){ ar * 0.5 + 0.25, 0.23}, (vec2){ 0.05, 0.05 }, 0);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        if(text_start + text_len < dp->lines) {
            pg_shader_2d_tex_frame(shader, 199);
            pg_shader_2d_transform(shader, (vec2){ ar * 0.5 + 0.25, 0.7 }, (vec2){ 0.05, 0.05 }, 0);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
    }
    if(d->menu.datapads.scroll_idx > 0) {
        pg_shader_2d_tex_frame(shader, 198);
        pg_shader_2d_transform(shader, (vec2){ 0.05, 0.2 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    if(d->menu.datapads.scroll_idx + 10 < d->num_held_datapads) {
        pg_shader_2d_tex_frame(shader, 199);
        pg_shader_2d_transform(shader, (vec2){ 0.05, 0.775 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
}

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
    int8_t* gmap = d->core->gpad_map;
    if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_CLOSED;
        SDL_ShowCursor(SDL_DISABLE);
        pg_mouse_mode(1);
        pg_audio_channel_pause(1, 0);
        pg_audio_channel_pause(2, 0);
    }
    int stick_ctrl_y = 0;
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
    }
    float ar = d->core->aspect_ratio;
    vec2 mouse_pos;
    int click;
    pg_mouse_pos(mouse_pos);
    vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0],
                                           1 / d->core->screen_size[1] });
    click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
    int scan_level = get_upgrade_level(d, BORK_UPGRADE_SCANNING);
    int dp_idx = 0, translate = 0, lines = 0;
    const struct bork_datapad* dp = NULL;
    if(d->menu.datapads.viewing_dp >= 0) {
        scan_level = get_upgrade_level(d, BORK_UPGRADE_SCANNING);
        dp_idx = d->held_datapads[d->menu.datapads.selection_idx];
        dp = &BORK_DATAPADS[dp_idx];
        if(dp->lines_translated && scan_level == 1) translate = 1;
        lines = translate ? dp->lines_translated : dp->lines;
    }
    if(click) {
        if(d->menu.datapads.viewing_dp >= 0) {
            if(vec2_dist(mouse_pos, (vec2){ 0.1 * ar, 0.35 }) < 0.04) {
                if(d->menu.datapads.text_scroll > 0) --d->menu.datapads.text_scroll;
            } else if(vec2_dist(mouse_pos, (vec2){ 0.1 * ar, 0.775 }) < 0.04) {
                if(d->menu.datapads.text_scroll < lines - 6) ++d->menu.datapads.text_scroll;
            } else if(fabs(mouse_pos[0] - (ar * 0.75 + 0.1)) < 0.2 && fabs(mouse_pos[1] - 0.875) < 0.05) {
                d->menu.datapads.viewing_dp = -1;
            }
        } else {
            /*  Clicking on the item list   */
            int i;
            for(i = 0; i < MIN(d->num_held_datapads, 10) ; ++i) {
                if(mouse_pos[1] > 0.2 + 0.06 * i && mouse_pos[1] < 0.2 + 0.06 + 0.06 * i
                && mouse_pos[0] > 0.1 && mouse_pos[0] < 1) {
                    d->menu.datapads.selection_idx = d->menu.datapads.scroll_idx + i;
                    d->menu.datapads.viewing_dp = d->menu.datapads.selection_idx;
                    d->menu.datapads.text_scroll = 0;
                    pg_audio_play(&d->core->menu_sound, 0.5);
                }
            }
            /*  Scrolling   */
            if(vec2_dist(mouse_pos, (vec2){ 0.05, 0.2 }) < 0.04
            && d->menu.datapads.scroll_idx > 0) {
                --d->menu.datapads.scroll_idx;
            } else if(vec2_dist(mouse_pos, (vec2){ 0.05, 0.775 }) < 0.04
                   && d->menu.datapads.scroll_idx < d->num_held_datapads - 10) {
                ++d->menu.datapads.scroll_idx;
            }
        }
    }
    if(d->menu.datapads.viewing_dp == -1) {
        if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)) {
            d->menu.datapads.scroll_idx = MAX(0, d->menu.datapads.scroll_idx - 1);
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
               && d->num_held_datapads > 10) {
            d->menu.datapads.scroll_idx = MIN(d->num_held_datapads - 10, d->menu.datapads.scroll_idx + 1);
        }
    } else {
        if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)) {
            d->menu.datapads.text_scroll = MAX(0, d->menu.datapads.text_scroll - 1);
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT) && lines > 6) {
            d->menu.datapads.text_scroll = MIN(lines - 6, d->menu.datapads.text_scroll + 1);
        }
    }
    if(d->menu.datapads.viewing_dp == -1 && d->num_held_datapads != 0) {
        int sel = d->menu.datapads.selection_idx;
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            d->menu.datapads.viewing_dp = d->menu.datapads.selection_idx;
            pg_audio_play(&d->core->menu_sound, 0.5);
        }
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.datapads.selection_idx =
                MIN(d->menu.datapads.selection_idx + 1, d->num_held_datapads - 1);
            if(d->menu.datapads.selection_idx >= d->menu.datapads.scroll_idx + 10)
                ++d->menu.datapads.scroll_idx;
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.datapads.selection_idx = MAX(d->menu.datapads.selection_idx - 1, 0);
            if(d->menu.datapads.selection_idx < d->menu.datapads.scroll_idx)
                --d->menu.datapads.scroll_idx;
        }
        if(sel != d->menu.datapads.selection_idx) d->menu.datapads.text_scroll = 0;
    } else if(d->menu.datapads.viewing_dp >= 0) {
        if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.datapads.viewing_dp = -1;
        }
        if(lines > 6) {
            if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1) {
                d->menu.datapads.text_scroll = MIN(d->menu.datapads.text_scroll + 1, lines - 6);
            } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
                d->menu.datapads.text_scroll = MAX(d->menu.datapads.text_scroll - 1, 0);
            }
        }
    }
}

void draw_datapad_menu(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_text;
    int scan_level = get_upgrade_level(d, BORK_UPGRADE_SCANNING);
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
    int dp_idx, translate = 0, lines = 0;
    const struct bork_datapad* dp;
    if(d->menu.datapads.viewing_dp == -1) {
        for(i = 0; i < list_len; ++i) {
            int dp_idx = d->held_datapads[d->menu.datapads.scroll_idx + i];
            const struct bork_datapad* dp = &BORK_DATAPADS[dp_idx];
            int is_selected = (i + d->menu.datapads.scroll_idx == d->menu.datapads.selection_idx);
            if(dp->lines_translated && scan_level == 1) {
                strncpy(text.block[++ti], dp->title_translated, 64);
            } else {
                strncpy(text.block[++ti], dp->title, 64);
            }
            vec4_set(text.block_style[ti], 0.1 + 0.05 * is_selected, 0.2 + 0.06 * i, 0.03, 1.2);
            vec4_set(text.block_color[ti], 1, 1, 1, d->menu.datapads.viewing_dp == -1 ? 1 : 0);
        }
    } else {
        scan_level = get_upgrade_level(d, BORK_UPGRADE_SCANNING);
        dp_idx = d->held_datapads[d->menu.datapads.selection_idx];
        dp = &BORK_DATAPADS[dp_idx];
        if(dp->lines_translated && scan_level == 1) translate = 1;
        lines = translate ? dp->lines_translated : dp->lines;
        float text_size = 0.02 * ar;
        for(i = 0; i < MIN(6, lines); ++i) {
            int draw_line = i + d->menu.datapads.text_scroll;
            int len = snprintf(text.block[++ti], 64, "%s",
                        translate ? dp->text_translated[draw_line] : dp->text[draw_line]);
            vec4_set(text.block_style[ti], ar * 0.5 - (len * text_size * 1.25 * 0.5),
                                             0.375 + 0.07 * i, text_size, 1.25);
            vec4_set(text.block_color[ti], 1, 1, 1, 0.95);
        }
        int len = snprintf(text.block[++ti], 8, "BACK");
        vec4_set(text.block_style[ti], ar * 0.75, 0.85, 0.05, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        len = snprintf(text.block[++ti], 32, "%s", translate ? dp->title_translated : dp->title);
        text_size *= 1.25;
        vec4_set(text.block_style[ti], ar * 0.5 - (len * text_size * 1.25 * 0.5),
                                        0.25, text_size, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(shader, &text);
    shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    if(d->menu.datapads.viewing_dp == -1) {
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
    } else {
        if(d->menu.datapads.text_scroll > 0) {
            pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
        } else pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.25 }, (vec4){});
        pg_shader_2d_tex_frame(shader, 198);
        pg_shader_2d_transform(shader, (vec2){ 0.1 * ar, 0.35 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        if(d->menu.datapads.text_scroll + 6 < lines) {
            pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
        } else pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.25 }, (vec4){});
        pg_shader_2d_tex_frame(shader, 199);
        pg_shader_2d_transform(shader, (vec2){ 0.1 * ar, 0.775 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
}

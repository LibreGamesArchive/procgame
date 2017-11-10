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

void recycler_count_inv_resources(struct bork_play_data* d)
{
    d->menu.recycler.resources[0] = 0;
    d->menu.recycler.resources[1] = 0;
    d->menu.recycler.resources[2] = 0;
    d->menu.recycler.resources[3] = 0;
    struct bork_entity* ent;
    bork_entity_t ent_id;
    int i;
    ARR_FOREACH(d->inventory, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent) continue;
        if(ent->flags & BORK_ENTFLAG_IS_FOOD)
            d->menu.recycler.resources[0] += ent->item_quantity;
        else if(ent->flags & BORK_ENTFLAG_IS_CHEMICAL)
            d->menu.recycler.resources[1] += ent->item_quantity;
        else if(ent->flags & BORK_ENTFLAG_IS_ELECTRICAL)
            d->menu.recycler.resources[2] += ent->item_quantity;
        else if(ent->flags & BORK_ENTFLAG_IS_RAW_MATERIAL)
            d->menu.recycler.resources[3] += ent->item_quantity;
    }
}

void recycler_consume_inv_resources(struct bork_play_data* d,
                                    int f, int c, int e, int r)
{
    struct bork_entity* ent;
    bork_entity_t ent_id;
    int i;
    ARR_FOREACH(d->inventory, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent) continue;
        if(f && ent->flags & BORK_ENTFLAG_IS_FOOD) {
            --f;
            remove_inventory_item(d, i);
            --i;
        } else if(c && ent->flags & BORK_ENTFLAG_IS_CHEMICAL) {
            --c;
            remove_inventory_item(d, i);
            --i;
        } else if(e && ent->flags & BORK_ENTFLAG_IS_ELECTRICAL) {
            --e;
            remove_inventory_item(d, i);
            --i;
        } else if(r && ent->flags & BORK_ENTFLAG_IS_RAW_MATERIAL) {
            --r;
            remove_inventory_item(d, i);
            --i;
        }
        if(f + c + e + r <= 0) return;
    }
}

void tick_recycler_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    recycler_count_inv_resources(d);
    if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_CLOSED;
        SDL_ShowCursor(SDL_DISABLE);
        pg_mouse_mode(1);
    }
    float ar = d->core->aspect_ratio;
    vec2 mouse_pos;
    int click;
    pg_mouse_pos(mouse_pos);
    vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0],
                                           1 / d->core->screen_size[1] });
    click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    if(click) {
        /*  Clicking on the item list   */
        int i;
        for(i = 0; i < 10; ++i) {
            if(mouse_pos[1] > 0.2 + 0.06 * i && mouse_pos[1] < 0.2 + 0.06 + 0.06 * i
            && mouse_pos[0] > 0.1 && mouse_pos[0] < 0.5) {
                d->menu.recycler.selection_idx = i + d->menu.recycler.scroll_idx;
            }
        }
        /*  Clicking the recycle button */
        enum bork_schematic sch = d->menu.recycler.selection_idx;
        const struct bork_schematic_detail* sch_d = &BORK_SCHEMATIC_DETAIL[sch];
        if(d->menu.recycler.obj && (d->held_schematics & (1 << sch))
        && fabs(mouse_pos[0] - (ar * 0.75 + 0.015)) < 0.25
        && fabs(mouse_pos[1] - 0.8) < 0.025
        && d->menu.recycler.resources[0] >= sch_d->resources[0]
        && d->menu.recycler.resources[1] >= sch_d->resources[1]
        && d->menu.recycler.resources[2] >= sch_d->resources[2]
        && d->menu.recycler.resources[3] >= sch_d->resources[3]) {
            recycler_consume_inv_resources(d,
                sch_d->resources[0], sch_d->resources[1],
                sch_d->resources[2], sch_d->resources[3]);
            bork_entity_t new_id = bork_entity_new(1);
            struct bork_entity* new_ent = bork_entity_get(new_id);
            bork_entity_init(new_ent, sch_d->product);
            vec3_set(new_ent->pos,
                d->looked_obj->recycler.out_pos[0] + (RANDF - 0.5) * 0.5,
                d->looked_obj->recycler.out_pos[1] + (RANDF - 0.5) * 0.5,
                d->looked_obj->recycler.out_pos[2]);
            bork_map_add_item(&d->map, new_id);
        }
        /*  Scrolling   */
        if(vec2_dist(mouse_pos, (vec2){ 0.05, 0.2 }) < 0.04
        && d->menu.recycler.scroll_idx > 0) {
            --d->menu.recycler.scroll_idx;
        } else if(vec2_dist(mouse_pos, (vec2){ 0.05, 0.775 }) < 0.04
               && d->menu.recycler.scroll_idx < BORK_NUM_SCHEMATICS - 10) {
            ++d->menu.recycler.scroll_idx;
        }
    }
    if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)) {
        d->menu.recycler.scroll_idx = MAX(0, d->menu.recycler.scroll_idx - 1);
    } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)) {
        d->menu.recycler.scroll_idx = MIN(BORK_NUM_SCHEMATICS - 10, d->menu.recycler.scroll_idx + 1);
    }
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT) || stick_ctrl_y == 1) {
        d->menu.recycler.selection_idx =
            MIN(d->menu.recycler.selection_idx + 1, BORK_NUM_SCHEMATICS - 1);
        if(d->menu.recycler.selection_idx >= d->menu.recycler.scroll_idx + 10)
            ++d->menu.recycler.scroll_idx;
    } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT) || stick_ctrl_y == -1) {
        d->menu.recycler.selection_idx = MAX(d->menu.recycler.selection_idx - 1, 0);
        if(d->menu.recycler.selection_idx < d->menu.recycler.scroll_idx)
            --d->menu.recycler.scroll_idx;
    }
    enum bork_schematic sch = d->menu.recycler.selection_idx;
    const struct bork_schematic_detail* sch_d = &BORK_SCHEMATIC_DETAIL[sch];
    if((pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT))
    && d->held_schematics & (1 << sch)) {
        if(d->menu.recycler.resources[0] >= sch_d->resources[0]
        && d->menu.recycler.resources[1] >= sch_d->resources[1]
        && d->menu.recycler.resources[2] >= sch_d->resources[2]
        && d->menu.recycler.resources[3] >= sch_d->resources[3]) {
            recycler_consume_inv_resources(d,
                sch_d->resources[0], sch_d->resources[1],
                sch_d->resources[2], sch_d->resources[3]);
            bork_entity_t new_id = bork_entity_new(1);
            struct bork_entity* new_ent = bork_entity_get(new_id);
            bork_entity_init(new_ent, sch_d->product);
            vec3_set(new_ent->pos,
                d->looked_obj->recycler.out_pos[0] + (RANDF - 0.5) * 0.75,
                d->looked_obj->recycler.out_pos[1] + (RANDF - 0.5) * 0.75,
                d->looked_obj->recycler.out_pos[2]);
            bork_map_add_item(&d->map, new_id);
        }
    }
}

void draw_recycler_menu(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    enum bork_schematic sch;
    const struct bork_schematic_detail* sch_d;
    const struct bork_entity_profile* prof;
    sch = d->menu.recycler.selection_idx;
    sch_d = &BORK_SCHEMATIC_DETAIL[sch];
    prof = &BORK_ENT_PROFILES[sch_d->product];
    int held = !!(d->held_schematics & (1 << sch));
    struct pg_shader_text text;
    if(d->menu.recycler.obj) {
        text = (struct pg_shader_text){
            .use_blocks = 2,
            .block = { "RECYCLER", "RECYCLE!" },
            .block_style = { { 0.1, 0.1, 0.05, 1.25 },
                             { ar * 0.75 + 0.015 - (8 * 0.04 * 1.25 * 0.5), 0.8, 0.04, 1.25 }},
            .block_color = { { 1, 1, 1, 0.7 },
                             { held ? 1 : 0.2, held ? 1 : 0.2,
                               held ? 1 : 0.2, held ? 1 : 0.9 } } };
    } else {
        text = (struct pg_shader_text){
            .use_blocks = 1,
            .block = { "SCHEMATICS" },
            .block_style = { { 0.1, 0.1, 0.05, 1.25 } },
            .block_color = { { 1, 1, 1, 0.7 } } };
    }
    int i;
    int ti = text.use_blocks;
    for(i = 0; i < 10; ++i) {
        sch = i + d->menu.recycler.scroll_idx;
        sch_d = &BORK_SCHEMATIC_DETAIL[sch];
        prof = &BORK_ENT_PROFILES[sch_d->product];
        int is_selected = (sch == d->menu.recycler.selection_idx);
        int is_available = !!(d->held_schematics & (1 << sch));
        strncpy(text.block[++ti], prof->name, 64);
        vec4_set(text.block_style[ti], 0.1 + 0.05 * is_selected, 0.2 + 0.06 * i, 0.03, 1.2);
        if(is_available) {
            vec4_set(text.block_color[ti], 1, 1, 1, 0.9);
        } else {
            vec4_set(text.block_color[ti], 0.2, 0.2, 0.2, 0.9);
        }
    }
    sch = d->menu.recycler.selection_idx;
    sch_d = &BORK_SCHEMATIC_DETAIL[sch];
    prof = &BORK_ENT_PROFILES[sch_d->product];
    for(i = 0; i < 4; ++i) {
        int len;
        if(d->held_schematics & (1 << sch)) {
            len = snprintf(text.block[++ti], 8, "%d/%d", d->menu.recycler.resources[i], sch_d->resources[i]);
        } else {
            len = snprintf(text.block[++ti], 8, "%d/?", d->menu.recycler.resources[i]);
        }
        float center = len * 0.03 * 1.2 * 0.5;
        vec4_set(text.block_style[ti], ar * 0.75 + 0.02 + 0.1 * (i - 1.5) * ar - center, 0.6, 0.03, 1.2);
        if((d->held_schematics & (1 << sch)) && sch_d->resources[i] > 0) {
            vec4_set(text.block_color[ti], 1, 1, 1, 0.9);
        } else {
            vec4_set(text.block_color[ti], 0.2, 0.2, 0.2, 0.9);
        }
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(shader, &text);
    shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    pg_shader_2d_tex_frame(shader, prof->front_frame);
    pg_shader_2d_add_tex_tx(shader, prof->sprite_tx, prof->sprite_tx + 2);
    vec2 light_pos = { sin((float)d->ticks / 180.0f / M_PI), cos((float)d->ticks / 180.0f / M_PI) };
    vec2_scale(light_pos, light_pos, 0.5);
    vec2_add(light_pos, light_pos, (vec2){ ar * 0.75, 0.3 });
    pg_shader_2d_set_light(shader, light_pos, (vec3){ 0.7, 0.7, 0.7 },
                           (vec3){ 0.2, 0.2, 0.2 });
    pg_shader_2d_transform(shader,
        (vec2){ ar * 0.75 + 0.015, 0.4 - (0.18 * prof->inv_height) },
        (vec2){ 0.10 * prof->sprite_tx[0] * ar, 0.10 * prof->sprite_tx[1] * ar },
        prof->inv_angle);
    if(d->held_schematics & (1 << sch)) {
        pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    } else {
        pg_shader_2d_color_mod(shader, (vec4){ 0, 0, 0, 1 }, (vec4){ 0.1, 0.1, 0.1, 0 });
    }
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    for(i = 0; i < 4; ++i) {
        pg_shader_2d_tex_frame(shader, 200 + i * 2);
        pg_shader_2d_add_tex_tx(shader, (vec2){ 2, 2 }, (vec2){});
        if((d->held_schematics & (1 << sch)) && sch_d->resources[i] > 0) {
            if(d->menu.recycler.resources[i] < sch_d->resources[i]) {
                pg_shader_2d_color_mod(shader, (vec4){ 1, 0.1, 0.1, 1 }, (vec4){});
            } else {
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
            }
        } else {
            pg_shader_2d_color_mod(shader, (vec4){ 0.2, 0.2, 0.2, 0.9 }, (vec4){});
        }
        pg_shader_2d_transform(shader, (vec2){ ar * 0.75 + 0.015 + 0.1 * ((i - 1.5) * ar), 0.7 }, (vec2){ 0.05, 0.05 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    if(d->menu.recycler.scroll_idx > 0) {
        pg_shader_2d_tex_frame(shader, 198);
        pg_shader_2d_transform(shader, (vec2){ 0.05, 0.2 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    if(d->menu.recycler.scroll_idx + 10 < BORK_NUM_SCHEMATICS) {
        pg_shader_2d_tex_frame(shader, 199);
        pg_shader_2d_transform(shader, (vec2){ 0.05, 0.775 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
}

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

/*  Menu/UI functions   */
void tick_control_inv_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_CLOSED;
        SDL_ShowCursor(SDL_DISABLE);
        pg_mouse_mode(1);
        return;
    } else if(pg_check_input(SDL_SCANCODE_TAB, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_UPGRADES;
        return;
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

void draw_menu_inv(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    vec2 mouse;
    pg_mouse_pos(mouse);
    vec2_scale(mouse, mouse, 1 / d->core->screen_size[1]);
    /*
    pg_shader_2d_set_light(shader, mouse, (vec3){ 0.5, 0.5, 0.5 },
                           (vec3){ 0.5, 0.5, 0.5 });
    */
    vec2 light_pos = { sin((float)d->ticks / 180.0f / M_PI), cos((float)d->ticks / 180.0f / M_PI) };
    vec2_scale(light_pos, light_pos, 0.5);
    vec2_add(light_pos, light_pos, (vec2){ ar * 0.75, 0.25 });
    pg_shader_2d_set_light(shader, light_pos, (vec3){ 0.7, 0.7, 0.7 },
                           (vec3){ 0.2, 0.2, 0.2 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
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
                    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.5 }, (vec4){});
                } else {
                    pg_shader_2d_transform(shader,
                        (vec2){ ar * 0.75 - 0.2 + 0.15 * i, 0.43 },
                        (vec2){ 0.075, 0.075 }, 0);
                    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
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

void draw_quickfetch_items(struct bork_play_data* d,
                           vec4 color_mod, vec4 selected_mod)
{
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    float w = d->core->aspect_ratio;
    pg_shader_2d_resolution(shader, (vec2){ w, 1 });
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_tex_frame(shader, 0);
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    vec2 light_pos = { sin((float)d->ticks / 180.0f / M_PI), cos((float)d->ticks / 180.0f / M_PI) };
    vec2_scale(light_pos, light_pos, 0.3);
    vec2_add(light_pos, light_pos, (vec2){ w - 0.2, 0.8 });
    pg_shader_2d_set_light(shader, light_pos, (vec3){ 0.7, 0.7, 0.7 },
                           (vec3){ 0.2, 0.2, 0.2 });
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
            pg_shader_2d_color_mod(shader, color_mod, (vec4){});
        } else {
            pg_shader_2d_color_mod(shader, selected_mod, (vec4){});
        }
        pg_shader_2d_transform(shader,
            (vec2){ draw_pos[i][0], draw_pos[i][1] - 0.05 * prof->inv_height },
            (vec2){ 0.05 * prof->sprite_tx[0], 0.05 * prof->sprite_tx[1] },
            prof->inv_angle);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
}

void draw_quickfetch_text(struct bork_play_data* d, int draw_label,
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

/*  Inventory management functions  */
bork_entity_t remove_inventory_item(struct bork_play_data* d, int inv_idx)
{
    if(inv_idx >= d->inventory.len) return -1;
    bork_entity_t item_id = d->inventory.data[inv_idx];
    struct bork_entity* item = bork_entity_get(item_id);
    if(!item) return -1;
    if(item->item_quantity > 1) {
        bork_entity_t rem_id = bork_entity_new(1);
        struct bork_entity* rem_item = bork_entity_get(rem_id);
        rem_item->item_quantity = 1;
        bork_entity_init(rem_item, item->type);
        --item->item_quantity;
        return rem_id;
    }
    ARR_SPLICE(d->inventory, inv_idx, 1);
    int i;
    for(i = 0; i < 4; ++i) {
        if(d->quick_item[i] == inv_idx) d->quick_item[i] = -1;
        else if(d->quick_item[i] > inv_idx) --d->quick_item[i];
    }
    if(d->held_item == inv_idx) d->held_item = -1;
    bork_entity_init(item, item->type);
    return item_id;
}

void add_inventory_item(struct bork_play_data* d, bork_entity_t ent_id)
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

void switch_item(struct bork_play_data* d, int inv_idx)
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

void set_quick_item(struct bork_play_data* d, int quick_idx, int inv_idx)
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

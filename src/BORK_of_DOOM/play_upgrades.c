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
#include "game_states.h"
#include "state_play.h"
#include "datapad_content.h"

/*  Gameplay tick functions for individual upgrades with active powers or
    constant effects. The HEATSHIELD and STRENGTH upgrades have to be checked
    for when the player may receive fire damage, or when the player is using
    a melee attack, so those two are not represented here   */
static void tick_jetpack(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(!(d->plr.flags & BORK_ENTFLAG_GROUND)
    && d->jump_released && d->upgrade_counters[idx] >= 3
    && pg_check_input(kmap[BORK_CTRL_JUMP], PG_CONTROL_HELD)) {
        d->upgrade_counters[idx] -= 3;
        if(l == 0) d->plr.vel[2] = MAX(-0.02, d->plr.vel[2]);
        else if(d->plr.vel[2] < 0.05) d->plr.vel[2] += 0.008;
    } else if(d->upgrade_counters[idx] < PLAY_SECONDS(10)) {
        ++d->upgrade_counters[idx];
    }
}

static void tick_doorhack(struct bork_play_data* d, int l, int idx)
{
    static int mouse_released = 1;
    if(d->upgrade_selected != idx) return;
    if(!mouse_released && pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_RELEASED))
        mouse_released = 1;
    if(!mouse_released) {
        d->upgrade_counters[idx] = 0;
        return;
    }
    if(pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_HELD)
    && d->looked_obj && d->looked_obj->type == BORK_MAP_DOORPAD) {
        struct bork_map_object* door = &d->map.doors.data[d->looked_obj->doorpad.door_idx];
        if(d->upgrade_use_level == 0 && door->door.locked == 1) {
            d->upgrade_counters[idx] = 0;
            return;
        } else if(d->upgrade_use_level == 1 && door->door.locked != 1) {
            d->upgrade_counters[idx] = 0;
            return;
        }
        d->upgrade_counters[idx] += 1;
        if(d->upgrade_counters[idx] >= PLAY_SECONDS(1)) {
            mouse_released = 0;
            d->upgrade_counters[idx] = 0;
            if(d->upgrade_use_level == 0) {
                if(door->door.locked == 2) door->door.locked = 0;
                else if(door->door.locked == 0) {
                    door->door.locked = 2;
                    door->door.open = 0;
                }
            } else if(d->upgrade_use_level == 1) {
                door->door.locked = 0;
            }
        }
    } else d->upgrade_counters[idx] = 0;
}

static void tick_bothack(struct bork_play_data* d, int l, int idx)
{
    static int mouse_released = 1;
    static bork_entity_t curr_enemy = -1;
    if(d->upgrade_selected != idx) return;
    if(!mouse_released && pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_RELEASED))
        mouse_released = 1;
    if(!mouse_released) {
        d->upgrade_counters[idx] = 0;
        return;
    }
    if(pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_HELD) && d->looked_enemy >= 0) {
        if(d->looked_enemy != curr_enemy) {
            d->upgrade_counters[idx] = 0;
            curr_enemy = d->looked_enemy;
        }
        struct bork_entity* vis_enemy = bork_entity_get(d->looked_enemy);
        if(!vis_enemy || vis_enemy->HP <= 0) {
            d->upgrade_counters[idx] = 0;
            curr_enemy = -1;
            return;
        }
        vec3 eye, dir;
        float vis_dist, dist;
        bork_entity_get_eye(&d->plr, dir, eye);
        vis_dist = bork_map_vis_dist(&d->map, eye, dir);
        dist = vec3_dist(eye, vis_enemy->pos);
        if(dist - 0.25 > vis_dist) {
            curr_enemy = -1;
            d->upgrade_counters[idx] = 0;
            return;
        } else {
            ++d->upgrade_counters[idx];
            if(l == 1 && d->upgrade_counters[idx] >= PLAY_SECONDS(3)) {
                mouse_released = 0;
                d->upgrade_counters[idx] = 0;
                vis_enemy->HP = 0;
                vis_enemy->flags |= BORK_ENTFLAG_ON_FIRE;
            } else if(l == 0 && d->upgrade_counters[idx] >= PLAY_SECONDS(1.5)) {
                mouse_released = 0;
                vis_enemy->flags |= BORK_ENTFLAG_EMP;
                vis_enemy->emp_ticks = PLAY_SECONDS(5);
            }
        }
    } else {
        d->upgrade_counters[idx] = 0;
    }
}

static void tick_decoy(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_counters[idx] <= 0) {
        if(d->upgrade_selected == idx
        && pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_HIT)) {
            vec3 dir, pos;
            bork_entity_get_eye(&d->plr, dir, pos);
            if(l == 0) {
                vec3_dup(d->decoy_pos, d->plr.pos);
                d->decoy_active = 1;
                d->upgrade_counters[idx] = PLAY_SECONDS(15);
            } else if(l == 1) {
                float vis_dist;
                vec3 vis;
                vis_dist = bork_map_vis_dist(&d->map, pos, dir);
                vec3_scale(vis, dir, vis_dist);
                vec3_sub(vis, vis, dir);
                vec3_add(vis, vis, pos);
                vec3_dup(d->decoy_pos, vis);
                d->decoy_active = 1;
                d->upgrade_counters[idx] = PLAY_SECONDS(15);
            }
            struct bork_collision coll = {};
            bork_map_collide(&d->map, &coll, d->decoy_pos, (vec3){ 0.9, 0.9, 0.9 });
            vec3_add(d->decoy_pos, d->decoy_pos, coll.push);
            bork_map_build_plr_dist(&d->map, d->decoy_pos);
        }
    } else {
        --d->upgrade_counters[idx];
        if(d->decoy_active && d->upgrade_counters[idx] <= PLAY_SECONDS(5)) {
            d->decoy_active = 0;
            bork_map_build_plr_dist(&d->map, d->plr.pos);
        }
    }
}

static void tick_healing(struct bork_play_data* d, int l, int idx)
{
    if(l == 1) {
        ++d->upgrade_counters[idx];
        if(d->upgrade_counters[idx] >= PLAY_SECONDS(0.5)) {
            d->upgrade_counters[idx] = 0;
            d->plr.HP = MIN(100, d->plr.HP + 2);
        }
    } else {
        if(d->upgrade_selected == idx && d->upgrade_counters[idx] <= 0
        && pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_HIT)) {
            d->plr.HP += 15;
            d->upgrade_counters[idx] = PLAY_SECONDS(8);
        }
        if(d->upgrade_counters[idx] > 0) --d->upgrade_counters[idx];
    }
}

static void tick_defense(struct bork_play_data* d, int l, int idx)
{
    int i;
    bork_entity_t ent_id;
    struct bork_entity* ent;
    if(l == 1) {
        if(d->ticks % PLAY_SECONDS(1) == 0) {
            d->upgrade_counters[idx] = 0;
            ARR_FOREACH(d->plr_enemy_query, ent_id, i) {
                ent = bork_entity_get(ent_id);
                if(!ent) continue;
                float dist = vec3_dist(d->plr.pos, ent->pos);
                if(dist < 5) {
                    ent->HP -= 2;
                    create_sparks(d, ent->pos, 3);
                }
            }
        }
    }
    if(d->upgrade_counters[idx] > 0) --d->upgrade_counters[idx];
    if(d->upgrade_selected == idx && d->upgrade_counters[idx] <= 0) {
        if(pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_HIT)) {
            d->upgrade_counters[idx] = PLAY_SECONDS(6);
            ARR_FOREACH(d->plr_enemy_query, ent_id, i) {
                ent = bork_entity_get(ent_id);
                if(!ent) continue;
                float dist = vec3_dist(d->plr.pos, ent->pos);
                if(dist < 5) {
                    ent->HP -= 15;
                    create_sparks(d, ent->pos, 3);
                    vec3 push;
                    vec3_sub(push, ent->pos, d->plr.pos);
                    vec3_set_len(push, push, 0.25);
                    push[2] += 0.05;
                    vec3_add(ent->vel, ent->vel, push);
                }
            }
        }
    }
}

void tick_upgrades(struct bork_play_data* d)
{
    int i;
    for(i = 0; i < 3; ++i) {
        if(d->upgrades[i] == -1) continue;
        switch(d->upgrades[i]) {
        case BORK_UPGRADE_JETPACK: tick_jetpack(d, d->upgrade_level[i], i); break;
        case BORK_UPGRADE_DOORHACK: tick_doorhack(d, d->upgrade_level[i], i); break;
        case BORK_UPGRADE_BOTHACK: tick_bothack(d, d->upgrade_level[i], i); break;
        case BORK_UPGRADE_DECOY: tick_decoy(d, d->upgrade_level[i], i); break;
        case BORK_UPGRADE_DEFENSE: tick_defense(d, d->upgrade_level[i], i); break;
        case BORK_UPGRADE_HEALING: tick_healing(d, d->upgrade_level[i], i); break;
        default: break;
        }
    }
}

/*  HUD drawing functions for each upgrade. Certain passive upgrades can
    just use the hud_passive function   */
void hud_passive(struct bork_play_data* d, int l, int idx, int passive_i)
{
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.3 + passive_i * 0.075, 0.885 }, (vec2){ 0.03, 0.03 }, 0);
    if(l == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, d->upgrades[idx]);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
}

void hud_jetpack(struct bork_play_data* d, int l, int idx, int passive_i)
{
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.3 + passive_i * 0.075, 0.885 }, (vec2){ 0.03, 0.03 }, 0);
    pg_shader_2d_color_mod(&d->core->shader_2d,
        (vec4){ 1, 1, 1, d->upgrade_counters[idx] / (float)PLAY_SECONDS(10) });
    if(l == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_JETPACK);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
}

void hud_doorhack(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_selected != idx) return;
    float scale = 1;
    if(d->upgrade_counters[idx] > 0) {
        scale = (d->upgrade_counters[idx] / (float)PLAY_SECONDS(1)) * 0.75 + 0.25;
    }
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.15, 0.85 },
                           (vec2){ 0.075 * scale, 0.075 * scale }, 0);
    if(d->upgrade_use_level == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_DOORHACK);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
}

void hud_bothack(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_selected != idx) return;
    float scale = 1;
    if(d->upgrade_counters[idx] > 0) {
        float s = (l == 1) ? PLAY_SECONDS(3) : PLAY_SECONDS(1.5);
        scale = (d->upgrade_counters[idx] / s) * 0.75 + 0.25;
    }
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.15, 0.85 },
                           (vec2){ 0.075 * scale, 0.075 * scale }, 0);
    if(d->looked_enemy == -1) {
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.25 });
    } else {
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    }
    if(d->upgrade_use_level == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_BOTHACK);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
}

void hud_decoy(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_selected == idx) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.15, 0.85 },
                               (vec2){ 0.075, 0.075 }, 0);
        float a = 1.0f;
        if(d->upgrade_counters[idx] > 0) {
            a = (1.0f - (d->upgrade_counters[idx] / (float)PLAY_SECONDS(15))) * 0.7 + 0.05;
        }
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, a });
        if(l == 1) {
            pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_DECOY);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    }
}

void hud_healing(struct bork_play_data* d, int l, int idx, int passive_i)
{
    if(l == 1) {
        hud_passive(d, l, idx, passive_i);
        return;
    }
    if(d->upgrade_selected == idx) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_HEALING);
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.15, 0.85 },
                               (vec2){ 0.075, 0.075 }, 0);
        float a = 1.0f;
        if(d->upgrade_counters[idx] > 0) {
            a = (1.0f - (d->upgrade_counters[idx] / (float)PLAY_SECONDS(8))) * 0.7 + 0.05;
        }
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, a });
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    }
}

void hud_defense(struct bork_play_data* d, int l, int idx, int passive_i)
{
    if(d->upgrade_selected == idx) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.15, 0.85 },
                               (vec2){ 0.075, 0.075 }, 0);
        float a = 1.0f;
        if(d->upgrade_counters[idx] > 0) {
            a = (1.0f - (d->upgrade_counters[idx] / (float)PLAY_SECONDS(6))) * 0.7 + 0.05;
        }
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, a });
        if(l == 1) {
            pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_DEFENSE);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    }
}

void draw_upgrade_hud(struct bork_play_data* d)
{
    pg_shader_begin(&d->core->shader_2d, NULL);
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d_ctr, &d->core->shader_2d);
    pg_shader_2d_texture(&d->core->shader_2d, &d->core->upgrades_tex);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    int i;
    int passive_i = 0;
    for(i = 0; i < 3; ++i) {
        if(d->upgrades[i] == -1) continue;
        switch(d->upgrades[i]) {
        case BORK_UPGRADE_JETPACK:
            hud_jetpack(d, d->upgrade_level[i], i, passive_i);
            ++passive_i;
            break;
        case BORK_UPGRADE_DOORHACK:
            hud_doorhack(d, d->upgrade_level[i], i);
            break;
        case BORK_UPGRADE_BOTHACK:
            hud_bothack(d, d->upgrade_level[i], i);
            break;
        case BORK_UPGRADE_DECOY:
            hud_decoy(d, d->upgrade_level[i], i);
            break;
        case BORK_UPGRADE_HEALING:
            hud_healing(d, d->upgrade_level[i], i, passive_i);
            if(d->upgrade_level[i] == 1) ++passive_i;
            break;
        case BORK_UPGRADE_DEFENSE:
            hud_defense(d, d->upgrade_level[i], i, passive_i);
            break;
        case BORK_UPGRADE_HEATSHIELD:
        case BORK_UPGRADE_STRENGTH:
            hud_passive(d, d->upgrade_level[i], i, passive_i);
            ++passive_i;
            break;
        default: break;
        }
    }
}

int get_upgrade_level(struct bork_play_data* d, enum bork_upgrade up)
{
    if(d->upgrades[0] == up) return d->upgrade_level[0];
    if(d->upgrades[1] == up) return d->upgrade_level[1];
    if(d->upgrades[2] == up) return d->upgrade_level[2];
    return -1;
}

#define CAN_INSTALL     1
#define CAN_UPGRADE     2
#define MUST_REPLACE    3
#define CONFIRM_REPLACE 4
#define CANNOT_UPGRADE  5
static int can_install(struct bork_play_data* d, int up_type)
{
    int i;
    int can = MUST_REPLACE;
    for(i = 0; i < 3; ++i) {
        if(d->upgrades[i] == up_type) {
            if(d->upgrade_level[i] == 0) {
                can = CAN_UPGRADE;
                break;
            } else if(d->upgrade_level[i] == 1) {
                can = CANNOT_UPGRADE;
                break;
            }
        } else if(d->upgrades[i] == -1) {
            can = CAN_INSTALL;
            break;
        }
    }
    d->menu.upgrades.replace_idx = i % 3;
    return can;
}

void tick_control_upgrade_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    if(pg_check_input(SDL_SCANCODE_TAB, PG_CONTROL_HIT)) {
        d->menu.state = BORK_MENU_INVENTORY;
        d->menu.upgrades.confirm = 0;
        return;
    }
    struct bork_entity* item = NULL;
    if(d->held_upgrades.len > 0) {
        item = bork_entity_get(d->held_upgrades.data[d->menu.upgrades.selection_idx]);
    }
    d->menu.upgrades.selection_idx =
        CLAMP(d->menu.upgrades.selection_idx, 0, d->held_upgrades.len - 1);
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    if(!d->menu.upgrades.confirm) {
        if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
                || stick_ctrl_y == 1) {
            d->menu.upgrades.selection_idx = MIN(d->menu.upgrades.selection_idx + 1,
                                                 d->held_upgrades.len - 1);
            if(d->menu.upgrades.selection_idx >= d->menu.upgrades.scroll_idx + 4)
                ++d->menu.upgrades.scroll_idx;
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
                || stick_ctrl_y == -1) {
            d->menu.upgrades.selection_idx = MAX(d->menu.upgrades.selection_idx - 1, 0);
            if(d->menu.upgrades.selection_idx < d->menu.upgrades.scroll_idx)
                --d->menu.upgrades.scroll_idx;
        } else if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
                || stick_ctrl_x == -1) {
            d->menu.upgrades.side = 0;
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
                || stick_ctrl_x == 1) {
            d->menu.upgrades.side = 1;
        } else if(item && pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)) {
            int can = can_install(d, item->counter[d->menu.upgrades.side]);
            d->menu.upgrades.confirm = can;
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
                || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)
                || pg_check_gamepad(SDL_CONTROLLER_BUTTON_LEFTSHOULDER, PG_CONTROL_HIT)) {
            d->menu.state = BORK_MENU_CLOSED;
            pg_mouse_mode(1);
            return;
        }
    } else if(d->menu.upgrades.confirm == MUST_REPLACE) {
        if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT) || stick_ctrl_x == -1) {
            d->menu.upgrades.replace_idx = MAX(0, d->menu.upgrades.replace_idx - 1);
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT) || stick_ctrl_x == 1) {
            d->menu.upgrades.replace_idx = MIN(2, d->menu.upgrades.replace_idx + 1);
        } else if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)) {
            d->menu.upgrades.confirm = CONFIRM_REPLACE;
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
            d->menu.upgrades.confirm = 0;
        }
    } else if(d->menu.upgrades.confirm == CONFIRM_REPLACE) {
        if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)) {
            d->upgrades[d->menu.upgrades.replace_idx] = item->counter[d->menu.upgrades.side];
            d->upgrade_level[d->menu.upgrades.replace_idx] = 0;
            ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
            d->menu.upgrades.confirm = 0;
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
            d->menu.upgrades.confirm = 0;
        }
    } else if(d->menu.upgrades.confirm == CAN_UPGRADE) {
        if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)) {
            d->upgrade_level[d->menu.upgrades.replace_idx] = 1;
            ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
            d->menu.upgrades.confirm = 0;
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
            d->menu.upgrades.confirm = 0;
        }
    } else if(d->menu.upgrades.confirm == CAN_INSTALL) {
        if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)) {
            d->upgrades[d->menu.upgrades.replace_idx] = item->counter[d->menu.upgrades.side];
            d->upgrade_level[d->menu.upgrades.replace_idx] = 0;
            ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
            d->menu.upgrades.confirm = 0;
        } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
            d->menu.upgrades.confirm = 0;
        }
    }
}

void draw_upgrade_menu(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    const struct bork_upgrade_detail* selected_upgrade = NULL;
    struct bork_entity* item;
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_shader_2d_texture(shader, &d->core->upgrades_tex);
    pg_shader_2d_set_light(shader, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    int inv_len = MIN(4, d->held_upgrades.len);
    int inv_start = d->menu.upgrades.scroll_idx;
    if(inv_start + inv_len > d->held_upgrades.len) {
        inv_start = d->held_upgrades.len - inv_len;
    }
    int i;
    for(i = 0; i < 3; ++i) {
        if(d->menu.upgrades.confirm != 0 && i == d->menu.upgrades.replace_idx) {
            pg_shader_2d_transform(shader,
                (vec2){ ar * 0.5 + (i - 1) * (0.15 * ar), 0.8 },
                (vec2){ 0.1, 0.1 }, 0);
        } else {
            pg_shader_2d_transform(shader,
                (vec2){ ar * 0.5 + (i - 1) * (0.15 * ar), 0.8 },
                (vec2){ 0.075, 0.075 }, 0);
        }
        if(d->upgrade_level[i] == 1) {
            pg_shader_2d_tex_frame(shader, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        if(d->upgrades[i] == -1) {
            pg_shader_2d_tex_frame(shader, 15);
        } else {
            pg_shader_2d_tex_frame(shader, d->upgrades[i]);
        }
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    for(i = 0; i < inv_len; ++i) {
        item = bork_entity_get(d->held_upgrades.data[i + inv_start]);
        if(!item) continue;
        enum bork_upgrade up0 = item->counter[0];
        enum bork_upgrade up1 = item->counter[1];
        const struct bork_upgrade_detail* up_d[2] =
            { bork_upgrade_detail(up0), bork_upgrade_detail(up1) };
        if(i + inv_start == d->menu.upgrades.selection_idx
        && d->menu.upgrades.side == 0) {
            selected_upgrade = up_d[0];
            pg_shader_2d_transform(shader,
                (vec2){ 0.2, 0.25 + (i * 0.125) }, (vec2){ 0.065, 0.065 }, 0);
            pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
        } else {
            if(i + inv_start == d->menu.upgrades.selection_idx)
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.8 });
            else
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.4 });
            pg_shader_2d_transform(shader,
                (vec2){ 0.2, 0.25 + (i * 0.125) }, (vec2){ 0.05, 0.05 }, 0);
        }
        pg_shader_2d_tex_frame(shader, item->counter[0]);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        if(i + inv_start == d->menu.upgrades.selection_idx
        && d->menu.upgrades.side == 1) {
            selected_upgrade = up_d[1];
            pg_shader_2d_transform(shader,
                (vec2){ 0.35, 0.25 + (i * 0.125) }, (vec2){ 0.065, 0.065 }, 0);
            pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
        } else {
            if(i + inv_start == d->menu.upgrades.selection_idx)
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.8 });
            else
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.4 });
            pg_shader_2d_transform(shader,
                (vec2){ 0.35, 0.25 + (i * 0.125) }, (vec2){ 0.05, 0.05 }, 0);
        }
        pg_shader_2d_tex_frame(shader, item->counter[1]);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    struct pg_shader_text text = {
        .use_blocks = inv_len + 1,
        .block = {
            "TECH", "INSTALLED"
        },
        .block_style = {
            { 0.1, 0.1, 0.05, 1.25 },
            { ar * 0.5 - (strlen("INSTALLED") * 0.05 * 1.25 * 0.5), 0.925, 0.05, 1.25 },
        },
        .block_color = {
            { 1, 1, 1, 0.7 }, { 1, 1, 1, 0.7 },
            { 1, 1, 1, 0.8 }, { 1, 1, 1, 0.8 }, { 1, 1, 1, 0.8 },
            { 1, 1, 1, 0.8 }, { 1, 1, 1, 0.8 }, { 1, 1, 1, 0.8 },
            { 1, 1, 1, 0.8 }, { 1, 1, 1, 0.8 }, { 1, 1, 1, 0.8 },
        },
    };
    int ti = 2;
    if(selected_upgrade) {
        int len = snprintf(text.block[ti], 32, "%s", selected_upgrade->name);
        vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.04 * 1.25 * 0.5), 0.2, 0.04, 1.25);
        for(i = 0; i < 8; ++i) {
            if(!selected_upgrade->description[i]) break;
            snprintf(text.block[++ti], 32, "%s", selected_upgrade->description[i]);
            vec4_set(text.block_style[ti], ar * 0.4, 0.3 + i * 0.035, 0.02, 1.25);
        }
    }
    static const char* confirm_strings[6][3] = {
        [0] = { "[SPACE] INSTALL TECH", "", "" },
        [CAN_INSTALL] = { "   ARE YOU SURE?",
                          "[SPACE] CONFIRM INSTALL",
                          "[ESCAPE] CANCEL" },
        [CAN_UPGRADE] = { "   ARE YOU SURE?",
                          "[SPACE] CONFIRM UPGRADE",
                          "[ESCAPE] CANCEL" },
        [MUST_REPLACE] = { "SELECT TECH TO REPLACE",
                           "[SPACE] SELECT",
                           "[ESCAPE] CANCEL" },
        [CONFIRM_REPLACE] = { "  ARE YOU SURE?",
                              "[SPACE] CONFIRM REPLACE",
                              "[ESCAPE] CANCEL" },
        [CANNOT_UPGRADE] = { "[SPACE] INSTALL TECH",
                             "TECH ALREADY INSTALLED", "" } };
    if(d->held_upgrades.len > 0) {
        strncpy(text.block[++ti], confirm_strings[d->menu.upgrades.confirm][0], 32);
        vec4_set(text.block_style[ti], ar * 0.35, 0.55, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        strncpy(text.block[++ti], confirm_strings[d->menu.upgrades.confirm][1], 32);
        vec4_set(text.block_style[ti], ar * 0.35, 0.6, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        strncpy(text.block[++ti], confirm_strings[d->menu.upgrades.confirm][2], 32);
        vec4_set(text.block_style[ti], ar * 0.35, 0.65, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(&d->core->shader_text, &text);
}

void select_next_upgrade(struct bork_play_data* d)
{
    int checks = 0;
    int i;
    if(d->upgrade_selected == -1) i = -1;
    else i = d->upgrade_selected * 2 + d->upgrade_use_level;
    while(checks < 6) {
        ++checks;
        i = (i + 1) % 6;
        int u = i / 2;
        int l = i % 2;
        const struct bork_upgrade_detail* up = bork_upgrade_detail(d->upgrades[u]);
        if(d->upgrades[u] == -1 || !up->active[l]
        || (l == 1 && d->upgrade_level[u] != 1)
        || (l == 0 && d->upgrade_level[u] == 1 && !up->keep_first)) continue;
        else {
            d->upgrade_selected = u;
            d->upgrade_use_level = l;
            return;
        }
    }
}

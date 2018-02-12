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

/*  Gameplay tick functions for individual upgrades with active powers or
    constant effects. The HEATSHIELD and STRENGTH upgrades have to be checked
    for when the player may receive fire damage, or when the player is using
    a melee attack, so those two are not represented here   */
static void tick_jetpack(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    int max_fuel = (l == 0 ? PLAY_SECONDS(3) : PLAY_SECONDS(9));
    int jump_ctrl = (!(d->plr.flags & BORK_ENTFLAG_GROUND) && d->jump_released &&
        (pg_check_input(kmap[BORK_CTRL_JUMP], PG_CONTROL_HELD)
         || pg_check_gamepad(gmap[BORK_CTRL_JUMP], PG_CONTROL_HELD)));
    int tech_ctrl = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HELD)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HELD));
    if(d->upgrade_counters[idx] >= 3 && (jump_ctrl || tech_ctrl)) {
        d->using_jetpack = 1;
        d->upgrade_counters[idx] -= 3;
        if(l == 0) {
            if(d->plr.vel[2] < 0.025) d->plr.vel[2] += 0.008;
            //d->plr.vel[2] = MAX(-0.02, d->plr.vel[2]);
        } else if(l == 1) {
            if(d->plr.vel[2] < 0.05) d->plr.vel[2] += 0.008;
        }
    } else if(d->upgrade_counters[idx] < max_fuel) {
        ++d->upgrade_counters[idx];
        d->using_jetpack = 0;
    }
}

static void tick_doorhack(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    static int mouse_released = 1;
    int pressed = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HELD)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HELD));
    if(d->upgrade_selected != idx) return;
    if(!mouse_released && !pressed) mouse_released = 1;
    if(!mouse_released) {
        d->upgrade_counters[idx] = 0;
        return;
    }
    if(pressed && d->looked_obj && d->looked_obj->type == BORK_MAP_DOORPAD) {
        struct bork_map_object* door = &d->map.doors.data[d->looked_obj->doorpad.door_idx];
        if(door->door.locked == 0) {
            hud_announce(d, "THIS DOOR IS ALREADY UNLOCKED");
            d->upgrade_counters[idx] = 0;
            mouse_released = 0;
            return;
        } else if(door->door.locked == 2) {
            hud_announce(d, "THIS DOOR HAS A SPECIAL SECURITY OVERRIDE");
            d->upgrade_counters[idx] = 0;
            mouse_released = 0;
            return;
        }
        d->upgrade_counters[idx] += 1;
        if(d->upgrade_counters[idx] >= PLAY_SECONDS(1)) {
            pg_audio_play_ch(&d->core->sounds[BORK_SND_HACK], 1, 1);
            door->door.locked = 0;
            mouse_released = 0;
            d->upgrade_counters[idx] = 0;
        }
    } else d->upgrade_counters[idx] = 0;
}

static void tick_bothack(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    int pressed = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT));
    if(d->upgrade_selected != idx) return;
    if(d->upgrade_counters[idx]) --d->upgrade_counters[idx];
    if(d->upgrade_counters[idx] <= 0 && pressed) {
        d->upgrade_counters[idx] = PLAY_SECONDS(8);
        pg_audio_play_ch(&d->core->sounds[BORK_SND_HACK], 1, 1);
        vec3 part_pos = {};
        vec3 eye, dir;
        float vis_dist, dist;
        bork_entity_get_eye(&d->plr, dir, eye);
        eye[2] -= 0.25;
        vis_dist = bork_map_vis_dist(&d->map, eye, dir, l == 1 ? 12 : 6);
        if(d->looked_enemy == -1) {
            vec3_set_len(dir, dir, vis_dist - 0.25);
            vec3_add(part_pos, eye, dir);
        } else {
            struct bork_entity* ent = bork_entity_get(d->looked_enemy);
            if(!ent) {
                vec3_set_len(dir, dir, vis_dist - 0.25);
                vec3_add(part_pos, eye, dir);
            } else {
                dist = vec3_dist(eye, ent->pos);
                if(dist - 0.25 > vis_dist) {
                    vec3_set_len(dir, dir, vis_dist - 0.25);
                    vec3_add(part_pos, eye, dir);
                } else {
                    vec3_set_len(dir, dir, dist - 0.25);
                    vec3_add(part_pos, eye, dir);
                    if(ent->type == BORK_ENEMY_LAIKA
                    || ent->type == BORK_ENEMY_GREAT_BANE) {
                        ent->flags |= BORK_ENTFLAG_EMP;
                        ent->emp_ticks = (l == 1) ? PLAY_SECONDS(7) : PLAY_SECONDS(4);
                    } else {
                        if(l == 1) ent->HP = 0;
                        else {
                            ent->flags |= BORK_ENTFLAG_EMP;
                            ent->emp_ticks = PLAY_SECONDS(5);
                        }
                    }
                }
            }
        }
        struct bork_particle new_part = {
            .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
            .pos = { part_pos[0], part_pos[1], part_pos[2] },
            .light = { 1.0, 1.0, 1.5, 2 },
            .vel = { 0, 0, 0 },
            .lifetime = 40,
            .ticks_left = 40,
            .frame_ticks = 8,
            .current_frame = 40,
            .start_frame = 40, .end_frame = 44,
        };
        ARR_PUSH(d->particles, new_part);
    }
}

static void tick_decoy(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    int pressed = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT));
    if(d->upgrade_counters[idx] <= 0) {
        if(d->upgrade_selected == idx && pressed) {
            pg_audio_play_ch(&d->core->sounds[BORK_SND_HACK], 1, 1);
            vec3 dir, pos;
            bork_entity_get_eye(&d->plr, dir, pos);
            if(l == 0) {
                vec3_dup(d->decoy_pos, d->plr.pos);
                d->decoy_active = 1;
                d->upgrade_counters[idx] = PLAY_SECONDS(11);
            } else if(l == 1) {
                float vis_dist;
                vec3 vis;
                vis_dist = bork_map_vis_dist(&d->map, pos, dir, 64);
                vec3_scale(vis, dir, vis_dist);
                vec3_sub(vis, vis, dir);
                vec3_add(vis, vis, pos);
                vec3_dup(d->decoy_pos, vis);
                d->decoy_active = 1;
                d->upgrade_counters[idx] = PLAY_SECONDS(12.5);
            }
            struct bork_collision coll = {};
            bork_map_collide(&d->map, &coll, d->decoy_pos, (vec3){ 0.9, 0.9, 0.9 });
            vec3_add(d->decoy_pos, d->decoy_pos, coll.push);
            bork_map_build_plr_dist(&d->map, d->decoy_pos);
        }
    } else {
        --d->upgrade_counters[idx];
        if(d->decoy_active && d->upgrade_counters[idx] <= PLAY_SECONDS(8)) {
            d->decoy_active = 0;
            bork_map_build_plr_dist(&d->map, d->plr.pos);
        }
    }
}

static void tick_healing(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    int pressed = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT));
    if(d->upgrade_counters[idx] > 0) --d->upgrade_counters[idx];
    if(d->plr.HP >= 100 || d->plr.HP <= 0) return;
    if(d->upgrade_counters[idx] <= 0) {
        d->upgrade_counters[idx] = (l == 1 ? PLAY_SECONDS(0.2) : PLAY_SECONDS(0.4));
        d->plr.HP = MIN(100, d->plr.HP + 1);
    }
}

static void tick_defense(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    int pressed = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT));
    int i;
    bork_entity_t ent_id;
    struct bork_entity* ent;
    if(d->upgrade_counters[idx] > 0) {
        --d->upgrade_counters[idx];
        return;
    }
    if(d->upgrade_selected == idx && d->upgrade_counters[idx] <= 0) {
        if(pressed) {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 1.0, 1.0, 1.5, 6 + l * 6 },
                .vel = { 0, 0, 0 },
                .lifetime = 40,
                .ticks_left = 40 };
            vec3_dup(new_part.pos, d->plr.pos);
            ARR_PUSH(d->particles, new_part);
            red_sparks(d, d->plr.pos, 0.45, rand() % 4 + 16 + (8 * l));
            blue_sparks(d, d->plr.pos, 0.45, rand() % 4 + 8 + (8 * l));
            pg_audio_play_ch(&d->core->sounds[BORK_SND_DEFENSE_FIELD], 1, 1);
            d->upgrade_counters[idx] = PLAY_SECONDS(6);
            float vis_dist;
            vec3 eye, dir;
            bork_entity_get_eye(&d->plr, NULL, eye);
            eye[2] -= 0.25;
            ARR_FOREACH(d->plr_enemy_query, ent_id, i) {
                ent = bork_entity_get(ent_id);
                if(!ent) continue;
                float dist = vec3_dist(ent->pos, eye);
                if(dist < 12) {
                    vec3_sub(dir, ent->pos, eye);
                    vec3_normalize(dir, dir);
                    vis_dist = bork_map_vis_dist(&d->map, eye, dir, 12);
                    if(dist - 0.5 > vis_dist) continue;
                    dist = 1 - (dist / 12);
                    dist = MAX(0.5, dist);
                    ent->HP -= (l == 1) ? 80 * dist : 40 * dist;
                    red_sparks(d, ent->pos, 0.1, 5);
                    if(ent->flags & BORK_ENTFLAG_STATIONARY) continue;
                    vec3_set_len(dir, dir, dist * 0.25);
                    dir[2] = MAX(0.1, dir[2]);
                    vec3_add(ent->vel, ent->vel, dir);
                }
            }
        }
    }
}

static void tick_scanning(struct bork_play_data* d, int l, int idx)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    int pressed = (pg_check_input(kmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT)
                    || pg_check_gamepad(gmap[BORK_CTRL_USE_TECH], PG_CONTROL_HIT));
    if(d->upgrade_selected == idx && pressed) {
        struct bork_entity* looked[3] = { bork_entity_get(d->looked_enemy),
                                          bork_entity_get(d->looked_entity),
                                          bork_entity_get(d->looked_item) };
        float dist[3] = {
            looked[0] ? vec3_dist(d->plr.pos, looked[0]->pos) : 32,
            looked[1] ? vec3_dist(d->plr.pos, looked[1]->pos) : 32,
            looked[2] ? vec3_dist(d->plr.pos, looked[2]->pos) : 32 };
        int closest = (dist[0] < dist[1] ? (dist[0] < dist[2] ? 0 : 2) : (dist[1] < dist[2] ? 1 : 2));
        float closest_dist = dist[closest];
        float obj_dist = d->looked_obj ? vec3_dist(d->plr.pos, d->looked_obj->pos) : 32;
        if(obj_dist < dist[closest]) {
            closest = 3;
            closest_dist = obj_dist;
        }
        if(closest_dist >= 32) return;
        vec3 eye, dir;
        float vis_dist;
        bork_entity_get_eye(&d->plr, dir, eye);
        vis_dist = bork_map_vis_dist(&d->map, eye, dir, 32);
        if(closest_dist - vis_dist > 0.5) return;
        if(closest == 3) {
            switch(d->looked_obj->type) {
                case BORK_MAP_DOOR: strncpy(d->scanned_name, "DOOR", 32); break;
                case BORK_MAP_TELEPORT: strncpy(d->scanned_name, "TELEPORTER", 32); break;
                case BORK_MAP_DOORPAD: strncpy(d->scanned_name, "KEYPAD", 32); break;
                case BORK_MAP_RECYCLER: strncpy(d->scanned_name, "RECYCLER", 32); break;
                case BORK_MAP_GRATE: strncpy(d->scanned_name, "GRATE", 32); break;
                default: d->scanned_name[0] = '\0'; break;
            }
        } else {
            const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[looked[closest]->type];
            strncpy(d->scanned_name, prof->name, 32);
        }
        d->scan_ticks = PLAY_SECONDS(3);
    } else if(d->scan_ticks > 0) --d->scan_ticks;
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
    tick_scanning(d, d->upgrade_level[3], 3);
}

/*  HUD drawing functions for each upgrade. Certain passive upgrades can
    just use the hud_passive function   */
void hud_passive(struct bork_play_data* d, int l, int idx, int passive_i)
{
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.425 + passive_i * 0.125, 0.885 }, (vec2){ 0.05, 0.05 }, 0);
    if(l == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, d->upgrades[idx]);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
}

void hud_jetpack(struct bork_play_data* d, int l, int idx, int passive_i)
{
    int max_fuel = (l == 0 ? PLAY_SECONDS(3) : PLAY_SECONDS(9));
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.425 + passive_i * 0.125, 0.885 }, (vec2){ 0.05, 0.05 }, 0);
    pg_shader_2d_color_mod(&d->core->shader_2d,
        (vec4){ 1, 1, 1, d->upgrade_counters[idx] / (float)max_fuel }, (vec4){});
    if(l == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_JETPACK);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
}

void hud_doorhack(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_selected != idx) return;
    float scale = 1;
    if(d->upgrade_counters[idx] > 0) {
        scale = (d->upgrade_counters[idx] / (float)PLAY_SECONDS(1)) * 0.75 + 0.25;
    }
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.2, 0.75 },
                           (vec2){ 0.1 * scale, 0.1 * scale }, 0);
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
    float a = 1.0f;
    if(d->upgrade_counters[idx] > 0) {
        a = (1.0f - (d->upgrade_counters[idx] / (float)PLAY_SECONDS(8))) * 0.7 + 0.05;
    }
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, a }, (vec4){});
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.2, 0.75 },
                           (vec2){ 0.1, 0.1 }, 0);
    if(d->upgrade_use_level == 1) {
        pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_BOTHACK);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
}

void hud_decoy(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_selected == idx) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.2, 0.75 },
                               (vec2){ 0.1, 0.1 }, 0);
        float a = 1.0f;
        if(d->upgrade_counters[idx] > 0) {
            a = (1.0f - (d->upgrade_counters[idx] / (float)PLAY_SECONDS(15))) * 0.7 + 0.05;
        }
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, a }, (vec4){});
        if(l == 1) {
            pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_DECOY);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
    }
}

void hud_defense(struct bork_play_data* d, int l, int idx, int passive_i)
{
    if(d->upgrade_selected == idx) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.2, 0.75 },
                               (vec2){ 0.1, 0.1 }, 0);
        float a = 1.0f;
        if(d->upgrade_counters[idx] > 0) {
            a = (1.0f - (d->upgrade_counters[idx] / (float)PLAY_SECONDS(6))) * 0.7 + 0.05;
        }
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, a }, (vec4){});
        if(l == 1) {
            pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        pg_shader_2d_tex_frame(&d->core->shader_2d, BORK_UPGRADE_DEFENSE);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
    }
}

void hud_scanning(struct bork_play_data* d, int l, int idx)
{
    if(d->upgrade_selected == idx) {
        pg_shader_2d_transform(&d->core->shader_2d, (vec2){ 0.2, 0.75 },
                               (vec2){ 0.1, 0.1 }, 0);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
        if(l == 1) {
            pg_shader_2d_tex_frame(&d->core->shader_2d, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        pg_shader_2d_tex_frame(&d->core->shader_2d, 9);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
    }
}


void draw_upgrade_hud(struct bork_play_data* d)
{
    pg_shader_begin(&d->core->shader_2d, NULL);
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d_ctr, &d->core->shader_2d);
    pg_shader_2d_texture(&d->core->shader_2d, &d->core->upgrades_tex);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 }, (vec4){});
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
        case BORK_UPGRADE_DEFENSE:
            hud_defense(d, d->upgrade_level[i], i, passive_i);
            break;
        case BORK_UPGRADE_HEALING:
        case BORK_UPGRADE_HEATSHIELD:
        case BORK_UPGRADE_STRENGTH:
            hud_passive(d, d->upgrade_level[i], i, passive_i);
            ++passive_i;
            break;
        default: break;
        }
    }
    hud_scanning(d, d->upgrade_level[3], i);
}

int get_upgrade_level(struct bork_play_data* d, enum bork_upgrade up)
{
    if(up == BORK_UPGRADE_SCANNING) return d->upgrade_level[3];
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
    if(up_type == BORK_UPGRADE_SCANNING) {
        if(d->upgrade_level[3] == 0) {
            d->menu.upgrades.replace_idx = 3;
            return CAN_UPGRADE;
        } else {
            return CANNOT_UPGRADE;
        }
    }
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
    d->menu.upgrades.replace_idx = i;
    return can;
}

void tick_control_upgrade_menu(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    struct bork_entity* item = NULL;
    if(d->held_upgrades.len > 0 && d->menu.upgrades.selection_idx < d->held_upgrades.len) {
        item = bork_entity_get(d->held_upgrades.data[d->menu.upgrades.selection_idx]);
    }
    d->menu.upgrades.selection_idx =
        CLAMP(d->menu.upgrades.selection_idx, 0, d->held_upgrades.len);
    int inv_len = MIN(4, d->held_upgrades.len);
    int inv_start = d->menu.upgrades.scroll_idx;
    int stick_ctrl_y = 0, stick_ctrl_x = 0;
    float ar = d->core->aspect_ratio;
    vec2 mouse_pos;
    int click;
    pg_mouse_pos(mouse_pos);
    vec2_mul(mouse_pos, mouse_pos, (vec2){ ar / d->core->screen_size[0],
                                           1 / d->core->screen_size[1] });
    click = pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT);
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl_y = SGN(stick[1]);
        else if(fabsf(stick[0]) > 0.6) stick_ctrl_x = SGN(stick[0]);
    }
    if(click) {
        int i;
        for(i = 0; i < inv_len; ++i) {
            vec2 up0_pos = { 0.2, 0.3 + (i * 0.125) };
            vec2 up1_pos = { 0.325, 0.325 + (i * 0.125) };
            if(vec2_dist(mouse_pos, up0_pos) < 0.06) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                d->menu.upgrades.selection_idx = inv_start + i;
                d->menu.upgrades.horiz_idx = 0;
                d->menu.upgrades.confirm = 0;
            } else if(vec2_dist(mouse_pos, up1_pos) < 0.06) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                d->menu.upgrades.selection_idx = inv_start + i;
                d->menu.upgrades.horiz_idx = 1;
                d->menu.upgrades.confirm = 0;
            }
        }
        for(i = 0; i < 4; ++i) {
            if(d->upgrades[MOD(i - 1, 4)] == -1) break;
            vec2 up_pos = { ar * 0.5 + (i - 1.5) * (0.15 * ar), 0.8 };
            if(vec2_dist(up_pos, mouse_pos) < 0.075) {
                if(i > 0 && d->menu.upgrades.confirm == MUST_REPLACE) {
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    d->menu.upgrades.replace_idx = i - 1;
                    d->menu.upgrades.confirm = CONFIRM_REPLACE;
                } else {
                    pg_audio_play(&d->core->menu_sound, 0.5);
                    d->menu.upgrades.selection_idx = d->held_upgrades.len;
                    d->menu.upgrades.horiz_idx = i;
                    d->menu.upgrades.confirm = 0;
                }
            }
        }
        if(d->menu.upgrades.confirm == 0) {
            if(vec2_dist(mouse_pos, (vec2){ 0.15, 0.2 }) < 0.04
            && d->menu.upgrades.scroll_idx > 0) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                --d->menu.upgrades.scroll_idx;
            } else if(vec2_dist(mouse_pos, (vec2){ 0.15, 0.775 }) < 0.04
                   && d->menu.upgrades.scroll_idx + 4 < d->held_upgrades.len) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                ++d->menu.upgrades.scroll_idx;
            } else if(fabs(mouse_pos[0] - (ar * 0.6)) < 0.2
                   && fabs(mouse_pos[1] - 0.565) < 0.02) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                int can = can_install(d, item->counter[d->menu.upgrades.horiz_idx]);
                d->menu.upgrades.confirm = can;
            }
        }
        if(d->menu.upgrades.confirm == CONFIRM_REPLACE
        || d->menu.upgrades.confirm == CAN_UPGRADE
        || d->menu.upgrades.confirm == CAN_INSTALL) {
            if(fabs(mouse_pos[0] - (ar * 0.55)) < 0.07
            && fabs(mouse_pos[1] - 0.615) < 0.03) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                if(d->menu.upgrades.confirm == CONFIRM_REPLACE) {
                    d->upgrades[d->menu.upgrades.replace_idx] = item->counter[d->menu.upgrades.horiz_idx];
                    d->upgrade_level[d->menu.upgrades.replace_idx] = 0;
                    ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
                    d->menu.upgrades.confirm = 0;
                } else if(d->menu.upgrades.confirm == CAN_UPGRADE) {
                    d->upgrade_level[d->menu.upgrades.replace_idx] = 1;
                    ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
                    d->menu.upgrades.confirm = 0;
                } else if(d->menu.upgrades.confirm == CAN_INSTALL) {
                    d->upgrades[d->menu.upgrades.replace_idx] = item->counter[d->menu.upgrades.horiz_idx];
                    d->upgrade_level[d->menu.upgrades.replace_idx] = 0;
                    ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
                    d->menu.upgrades.confirm = 0;
                    show_tut_message(d, BORK_TUT_SWITCH_UPGRADE);
                }
            } else if(fabs(mouse_pos[0] - (ar * 0.65)) < 0.05
            && fabs(mouse_pos[1] - 0.615) < 0.03) {
                pg_audio_play(&d->core->menu_sound, 0.5);
                d->menu.upgrades.confirm = 0;
            }
        }
    } else {
        if(d->menu.upgrades.confirm == CONFIRM_REPLACE
        || d->menu.upgrades.confirm == CAN_UPGRADE
        || d->menu.upgrades.confirm == CAN_INSTALL) {
            if(fabs(mouse_pos[0] - (ar * 0.55)) < 0.07
            && fabs(mouse_pos[1] - 0.615) < 0.03) {
                d->menu.upgrades.confirm_opt = 0;
            } else if(fabs(mouse_pos[0] - (ar * 0.65)) < 0.05
            && fabs(mouse_pos[1] - 0.615) < 0.03) {
                d->menu.upgrades.confirm_opt = 1;
            }
            if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
            || stick_ctrl_x == -1) {
                d->menu.upgrades.confirm_opt = 0;
            } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
                   || stick_ctrl_x == 1) {
                d->menu.upgrades.confirm_opt = 1;
            } else if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)
                   || pg_check_gamepad(SDL_CONTROLLER_BUTTON_B, PG_CONTROL_HIT)) {
                d->menu.upgrades.confirm = 0;
            }
        }
    }
    if(!d->menu.upgrades.confirm) {
        if(pg_check_input(PG_MOUSEWHEEL_UP, PG_CONTROL_HIT)) {
            d->menu.upgrades.scroll_idx = MAX(0, d->menu.upgrades.scroll_idx - 1);
        } else if(pg_check_input(PG_MOUSEWHEEL_DOWN, PG_CONTROL_HIT)
               && d->held_upgrades.len > 4) {
            d->menu.upgrades.scroll_idx = MIN(d->held_upgrades.len - 4, d->menu.upgrades.scroll_idx + 1);
        } else if(pg_check_input(kmap[BORK_CTRL_DOWN], PG_CONTROL_HIT)
                || stick_ctrl_y == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.upgrades.selection_idx = MIN(d->menu.upgrades.selection_idx + 1,
                                                 d->held_upgrades.len);
            if(d->menu.upgrades.selection_idx == d->held_upgrades.len)
                d->menu.upgrades.horiz_idx = 0;
            else if(d->menu.upgrades.selection_idx >= d->menu.upgrades.scroll_idx + 4)
                ++d->menu.upgrades.scroll_idx;
        } else if(pg_check_input(kmap[BORK_CTRL_UP], PG_CONTROL_HIT)
                || stick_ctrl_y == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.upgrades.selection_idx = MAX(d->menu.upgrades.selection_idx - 1, 0);
            if(d->menu.upgrades.selection_idx < d->menu.upgrades.scroll_idx)
                --d->menu.upgrades.scroll_idx;
            d->menu.upgrades.horiz_idx = MIN(1, d->menu.upgrades.horiz_idx);
        } else if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT)
                || stick_ctrl_x == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.upgrades.selection_idx == d->held_upgrades.len) {
                d->menu.upgrades.horiz_idx = MAX(0, d->menu.upgrades.horiz_idx - 1);
            } else d->menu.upgrades.horiz_idx = 0;
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT)
                || stick_ctrl_x == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.upgrades.selection_idx == d->held_upgrades.len) {
                d->menu.upgrades.horiz_idx = MIN(4, d->menu.upgrades.horiz_idx + 1);
                if(d->upgrades[MOD(d->menu.upgrades.horiz_idx - 1, 4)] == -1)
                    --d->menu.upgrades.horiz_idx;
            } else d->menu.upgrades.horiz_idx = 1;
        } else if(item && (pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT))) {
            int can = can_install(d, item->counter[d->menu.upgrades.horiz_idx]);
            d->menu.upgrades.confirm = can;
        } else if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.state = BORK_MENU_CLOSED;
            SDL_ShowCursor(SDL_DISABLE);
            pg_mouse_mode(1);
            pg_audio_channel_pause(1, 0);
            pg_audio_channel_pause(2, 0);
            return;
        }
    } else if(d->menu.upgrades.confirm == MUST_REPLACE) {
        if(pg_check_input(kmap[BORK_CTRL_LEFT], PG_CONTROL_HIT) || stick_ctrl_x == -1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.upgrades.replace_idx = MAX(0, d->menu.upgrades.replace_idx - 1);
        } else if(pg_check_input(kmap[BORK_CTRL_RIGHT], PG_CONTROL_HIT) || stick_ctrl_x == 1) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.upgrades.replace_idx = MIN(2, d->menu.upgrades.replace_idx + 1);
        } else if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.upgrades.confirm = CONFIRM_REPLACE;
        } else if(pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
               || pg_check_gamepad(gmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            d->menu.upgrades.confirm = 0;
        }
    } else if(d->menu.upgrades.confirm == CONFIRM_REPLACE) {
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.upgrades.confirm_opt == 1) d->menu.upgrades.confirm = 0;
            else {
                d->upgrades[d->menu.upgrades.replace_idx] = item->counter[d->menu.upgrades.horiz_idx];
                d->upgrade_level[d->menu.upgrades.replace_idx] = 0;
                ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
                d->menu.upgrades.confirm = 0;
            }
        }
    } else if(d->menu.upgrades.confirm == CAN_UPGRADE) {
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.upgrades.confirm_opt == 1) d->menu.upgrades.confirm = 0;
            else {
                d->upgrade_level[d->menu.upgrades.replace_idx] = 1;
                ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
                d->menu.upgrades.confirm = 0;
            }
        }
    } else if(d->menu.upgrades.confirm == CAN_INSTALL) {
        if(pg_check_input(kmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)
        || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
            pg_audio_play(&d->core->menu_sound, 0.5);
            if(d->menu.upgrades.confirm_opt == 1) d->menu.upgrades.confirm = 0;
            else {
                d->upgrades[d->menu.upgrades.replace_idx] = item->counter[d->menu.upgrades.horiz_idx];
                d->upgrade_level[d->menu.upgrades.replace_idx] = 0;
                ARR_SPLICE(d->held_upgrades, d->menu.upgrades.selection_idx, 1);
                d->menu.upgrades.confirm = 0;
            }
        }
    }
}

void draw_upgrade_menu(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    const struct bork_upgrade_detail* selected_upgrade = NULL;
    enum bork_upgrade selected_upgrade_type = -1;
    struct bork_entity* item;
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1 });
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_set_light(shader, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    int inv_len = MIN(4, d->held_upgrades.len);
    int inv_start = d->menu.upgrades.scroll_idx;
    if(inv_start > 0) {
        pg_shader_2d_tex_frame(shader, 198);
        pg_shader_2d_transform(shader, (vec2){ 0.15, 0.2 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    if(inv_start + 4 < d->held_upgrades.len) {
        pg_shader_2d_tex_frame(shader, 199);
        pg_shader_2d_transform(shader, (vec2){ 0.15, 0.775 }, (vec2){ 0.04, 0.04 }, 0);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
    }
    if(inv_start + inv_len > d->held_upgrades.len) {
        inv_start = d->held_upgrades.len - inv_len;
    }
    pg_shader_2d_texture(shader, &d->core->upgrades_tex);
    int i;
    for(i = 0; i < 4; ++i) {
        int up_i = MOD(i - 1, 4);
        if((d->menu.upgrades.confirm != 0 && up_i == d->menu.upgrades.replace_idx)
        || (d->menu.upgrades.selection_idx == d->held_upgrades.len
            && d->menu.upgrades.confirm == 0 && i == d->menu.upgrades.horiz_idx)) {
            pg_shader_2d_transform(shader,
                (vec2){ ar * 0.5 + (i - 1.5) * (0.15 * ar), 0.8 },
                (vec2){ 0.1, 0.1 }, 0);
        } else {
            pg_shader_2d_transform(shader,
                (vec2){ ar * 0.5 + (i - 1.5) * (0.15 * ar), 0.8 },
                (vec2){ 0.075, 0.075 }, 0);
        }
        if(d->upgrade_level[up_i] == 1) {
            pg_shader_2d_tex_frame(shader, 14);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
        if(d->upgrades[up_i] == -1) {
            pg_shader_2d_tex_frame(shader, 15);
        } else {
            pg_shader_2d_tex_frame(shader, d->upgrades[up_i]);
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
        && d->menu.upgrades.horiz_idx == 0) {
            selected_upgrade = up_d[0];
            selected_upgrade_type = item->counter[0];
            pg_shader_2d_transform(shader,
                (vec2){ 0.2, 0.3 + (i * 0.125) }, (vec2){ 0.065, 0.065 }, 0);
            pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
        } else {
            if(i + inv_start == d->menu.upgrades.selection_idx)
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.8 }, (vec4){});
            else
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.4 }, (vec4){});
            pg_shader_2d_transform(shader,
                (vec2){ 0.2, 0.3 + (i * 0.125) }, (vec2){ 0.05, 0.05 }, 0);
        }
        pg_shader_2d_tex_frame(shader, item->counter[0]);
        pg_model_draw(&d->core->quad_2d_ctr, NULL);
        if(i + inv_start == d->menu.upgrades.selection_idx
        && d->menu.upgrades.horiz_idx == 1) {
            selected_upgrade = up_d[1];
            selected_upgrade_type = item->counter[1];
            pg_shader_2d_transform(shader,
                (vec2){ 0.325, 0.325 + (i * 0.125) }, (vec2){ 0.065, 0.065 }, 0);
            pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 }, (vec4){});
        } else {
            if(i + inv_start == d->menu.upgrades.selection_idx)
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.8 }, (vec4){});
            else
                pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.4 }, (vec4){});
            pg_shader_2d_transform(shader,
                (vec2){ 0.325, 0.325 + (i * 0.125) }, (vec2){ 0.05, 0.05 }, 0);
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
        vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.04 * 1.25 * 0.5), 0.225, 0.04, 1.25);
        for(i = 0; i < 8; ++i) {
            if(!selected_upgrade->description[i]) break;
            len = snprintf(text.block[++ti], 32, "%s", selected_upgrade->description[i]);
            vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.02 * 1.25 * 0.5),
                                           0.3 + i * 0.035, 0.02, 1.25);
        }
    } else if(d->menu.upgrades.selection_idx == d->held_upgrades.len) {
        selected_upgrade = bork_upgrade_detail(d->upgrades[MOD(d->menu.upgrades.horiz_idx - 1, 4)]);
        int len = snprintf(text.block[ti], 32, "%s", selected_upgrade->name);
        vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.04 * 1.25 * 0.5), 0.2, 0.04, 1.25);
        for(i = 0; i < 8; ++i) {
            if(!selected_upgrade->description[i]) break;
            len = snprintf(text.block[++ti], 32, "%s", selected_upgrade->description[i]);
            vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.02 * 1.25 * 0.5),
                                           0.3 + i * 0.035, 0.02, 1.25);
        }
    }
    if(d->menu.upgrades.confirm == 0 && d->menu.upgrades.selection_idx < d->held_upgrades.len) {
        int len;
        if(selected_upgrade && get_upgrade_level(d, selected_upgrade_type) >= 0)
            len = snprintf(text.block[++ti], 16, "UPGRADE");
        else len = snprintf(text.block[++ti], 16, "INSTALL");
        vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.03 * 1.25 * 0.5), 0.55, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
    } else if(d->menu.upgrades.confirm == MUST_REPLACE) {
        int len = snprintf(text.block[++ti], 32, "SELECT TECH TO REPLACE");
        vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.03 * 1.25 * 0.5), 0.55, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
    } else if(d->menu.upgrades.confirm == CONFIRM_REPLACE
           || d->menu.upgrades.confirm == CAN_UPGRADE
           || d->menu.upgrades.confirm == CAN_INSTALL) {
        int len = snprintf(text.block[++ti], 16, "ARE YOU SURE?");
        vec4_set(text.block_style[ti], ar * 0.6 - (len * 0.03 * 1.25 * 0.5), 0.55, 0.03, 1.25);
        vec4_set(text.block_color[ti], 1, 1, 1, 1);
        len = snprintf(text.block[++ti], 16, "YES");
        vec4_set(text.block_style[ti], ar * 0.55 - (len * 0.03 * 1.25 * 0.5), 0.6, 0.03, 1.25);
        if(d->menu.upgrades.confirm_opt)
            vec4_set(text.block_color[ti], 0.2, 0.2, 0.2, 0.9);
        else vec4_set(text.block_color[ti], 1, 1, 1, 1);
        len = snprintf(text.block[++ti], 16, "NO");
        vec4_set(text.block_style[ti], ar * 0.65 - (len * 0.03 * 1.25 * 0.5), 0.6, 0.03, 1.25);
        if(!d->menu.upgrades.confirm_opt)
            vec4_set(text.block_color[ti], 0.2, 0.2, 0.2, 0.9);
        else vec4_set(text.block_color[ti], 1, 1, 1, 1);
    }
    text.use_blocks = ti + 1;
    pg_shader_text_write(&d->core->shader_text, &text);
}

void select_next_upgrade(struct bork_play_data* d, int n)
{
    int checks = 0;
    int i;
    if(d->upgrade_selected == -1) i = -1;
    else i = d->upgrade_selected * 2 + d->upgrade_use_level;
    while(checks < 8) {
        ++checks;
        i = MOD(i + n, 8);
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
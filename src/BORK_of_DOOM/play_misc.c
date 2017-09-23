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

#define RANDF   ((float)rand() / RAND_MAX)

void entity_on_fire(struct bork_play_data* d, struct bork_entity* ent)
{
    if(ent->fire_ticks % 41 == 0) {
        create_smoke(d,
            (vec3){ ent->pos[0] + (RANDF - 0.5),
                    ent->pos[1] + (RANDF - 0.5),
                    ent->pos[2] + (RANDF - 0.5) },
            (vec3){}, 180);
    } else if(ent->fire_ticks % 31 == 0) {
        struct bork_particle new_part = {
            .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT | BORK_PARTICLE_DECELERATE,
            .pos = { ent->pos[0] + (RANDF - 0.5),
                     ent->pos[1] + (RANDF - 0.5),
                     ent->pos[2] + (RANDF - 0.5) },
            .vel = { 0, 0, -0.005 },
            .ticks_left = 120,
            .frame_ticks = 0,
            .current_frame = 24 + rand() % 4,
        };
        ARR_PUSH(d->particles, new_part);
    } else if(ent->fire_ticks % 90 == 0) {
        ent->HP -= 8;
    }
    --ent->fire_ticks;
    if(ent->fire_ticks == 0) ent->flags &= ~BORK_ENTFLAG_ON_FIRE;
}

/*  Update code */
void bork_play_reset_hud_anim(struct bork_play_data* d)
{
    struct bork_entity* held_item;
    const struct bork_entity_profile* prof;
    d->reload_ticks = 0;
    d->reload_length = 0;
    d->hud_anim_progress = 0;
    d->hud_anim_speed = 0;
    d->hud_anim_active = 0;
    d->hud_anim_destroy_when_finished = 0;
    d->hud_callback_frame = -1;
    d->hud_anim_callback = NULL;
    vec3_set(d->hud_anim[0], 0, 0, 0);
    vec3_set(d->hud_anim[1], 0, 0, 0);
    vec3_set(d->hud_anim[2], 0, 0, 0);
    vec3_set(d->hud_anim[3], 0, 0, 0);
    vec3_set(d->hud_anim[4], 0, 0, 0);
    d->hud_angle[1] = 0;
    d->hud_angle[2] = 0;
    d->hud_angle[3] = 0;
    d->hud_angle[4] = 0;
    if(d->held_item >= 0
    && (held_item = bork_entity_get(d->inventory.data[d->held_item]))) {
        prof = &BORK_ENT_PROFILES[held_item->type];
        d->hud_angle[0] = prof->hud_angle;
    } else d->hud_angle[0] = 0;
}

bork_entity_t get_looked_item(struct bork_play_data* d)
{
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    int i;
    bork_entity_t looked_id = -1;
    bork_entity_t ent_id;
    struct bork_entity* ent = NULL;
    float closest_angle = 0.3f, closest_dist = 2.5f;
    ARR_FOREACH(d->plr_item_query, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent || ent->flags & BORK_ENTFLAG_NOT_INTERACTIVE) continue;
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, ent->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        looked_id = ent_id;
    }
    return looked_id;
}

bork_entity_t get_looked_enemy(struct bork_play_data* d)
{
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    int i;
    bork_entity_t looked_id = -1;
    bork_entity_t ent_id;
    struct bork_entity* ent = NULL;
    float closest_angle = 1.0f, closest_dist = 25.0f;
    ARR_FOREACH(d->plr_enemy_query, ent_id, i) {
        ent = bork_entity_get(ent_id);
        if(!ent) continue;
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, ent->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir)) * dist;
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        looked_id = ent_id;
    }
    ent = bork_entity_get(looked_id);
    float vis_dist = bork_map_vis_dist(&d->map, look_pos, look_dir);
    if(vis_dist + 0.25 < closest_dist) return -1;
    else return looked_id;
}

struct bork_map_object* get_looked_map_object(struct bork_play_data* d)
{
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    int i;
    struct bork_map_object* looked_obj = NULL;
    struct bork_map_object* obj;
    float closest_angle = 0.3f, closest_dist = 2.5f;
    ARR_FOREACH_PTR(d->map.doorpads, obj, i) {
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, obj->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        closest_angle = angle;
        looked_obj = obj;
    }
    ARR_FOREACH_PTR(d->map.recyclers, obj, i) {
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, obj->pos, look_pos);
        float dist = vec3_len(ent_to_plr);
        if(dist >= closest_dist) continue;
        vec3_normalize(ent_to_plr, ent_to_plr);
        float angle = acosf(vec3_mul_inner(ent_to_plr, look_dir));
        if(angle >= closest_angle) continue;
        closest_dist = dist;
        closest_angle = angle;
        looked_obj = obj;
    }
    return looked_obj;
}

void get_plr_pos_for_ai(struct bork_play_data* d, vec3 out)
{
    if(d->decoy_active) vec3_dup(out, d->decoy_pos);
    else {
        vec3_dup(out, d->plr.pos);
        if(d->plr.flags & BORK_ENTFLAG_CROUCH) out[2] += 0.45;
        else out[2] += 0.8;
    }
}

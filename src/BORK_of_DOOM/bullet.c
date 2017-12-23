#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "bullet.h"
#include "map_area.h"
#include "physics.h"
#include "particle.h"
#include "upgrades.h"
#include "recycler.h"
#include "state_play.h"
#include "game_states.h"

void bork_bullet_init(struct bork_bullet* blt, vec3 pos, vec3 dir)
{
    *blt = (struct bork_bullet) {
        .pos = { pos[0], pos[1], pos[2] },
        .dir = { dir[0], dir[1], dir[2] } };
}

static void bullet_die(struct bork_bullet* blt, struct bork_play_data* d)
{
    static bork_entity_arr_t surr = {};
    switch(blt->type) {
        case 0: case 1: case 2: case 3: case 4: case 5: {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_SPRITE,
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 15,
                .frame_ticks = 3,
                .start_frame = 0, .end_frame = 4
            };
            ARR_PUSH(d->particles, new_part);
            float dist = vec3_dist(blt->pos, d->plr.pos);
            if(dist < 8) {
                dist = 1 - (dist / 8);
                pg_audio_play(&d->core->sounds[BORK_SND_BULLET_HIT], dist * 0.5);
            }
            break;
        } case 30: {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 1.5, 0.5, 0.5, 2.5 },
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 45,
                .lifetime = 45
            };
            ARR_PUSH(d->particles, new_part);
            pg_audio_play(&d->core->sounds[BORK_SND_PLAZMA_HIT], 1);
            break;
        } case 31: {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_SPRITE,
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 15,
                .frame_ticks = 3,
                .start_frame = 0, .end_frame = 4
            };
            ARR_PUSH(d->particles, new_part);
            pg_audio_play(&d->core->sounds[BORK_SND_BULLET_HIT], 1);
            break;
        } case 6: {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 1.5, 0.5, 0.5, 2 },
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 30,
                .lifetime = 30
            };
            ARR_PUSH(d->particles, new_part);
            red_sparks(d, blt->pos, 0.25, rand() % 4 + 4);
            float dist = vec3_dist(blt->pos, d->plr.pos);
            if(dist < 12) {
                dist = 1 - (dist / 12);
                pg_audio_play(&d->core->sounds[BORK_SND_PLAZMA_HIT], dist);
            }
            break;
        } case 7: {
            ARR_TRUNCATE(surr, 0);
            vec3 surr_start, surr_end;
            vec3_sub(surr_start, blt->pos, (vec3){ 3, 3, 3 });
            vec3_add(surr_end, blt->pos, (vec3){ 3, 3, 3 });
            bork_map_query_enemies(&d->map, &surr, surr_start, surr_end);
            bork_map_query_entities(&d->map, &surr, surr_start, surr_end);
            int i;
            bork_entity_t ent_id;
            struct bork_entity* surr_ent;
            ARR_FOREACH(surr, ent_id, i) {
                surr_ent = bork_entity_get(ent_id);
                if(!surr_ent) continue;
                if(surr_ent->flags & BORK_ENTFLAG_STATIONARY) continue;
                vec3 push;
                vec3_sub(push, surr_ent->pos, blt->pos);
                float dist = vec3_len(push);
                float dist_f = MAX(1 - (dist / 3.0f), 0);
                surr_ent->HP -= dist_f * 50;
                vec3_set_len(push, push, 0.2 * dist_f);
                push[2] += 0.025;
                vec3_add(surr_ent->vel, surr_ent->vel, push);
            }
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 1.5, 0.5, 0.5, 4 },
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 45,
                .lifetime = 45
            };
            ARR_PUSH(d->particles, new_part);
            red_sparks(d, blt->pos, 0.45, rand() % 4 + 12);
            float dist = vec3_dist(blt->pos, d->plr.pos);
            if(dist < 16) {
                dist = 1 - (dist / 16);
                pg_audio_play(&d->core->sounds[BORK_SND_PLAZMA_HIT], dist * 2);
            }
            break;
        } case 8: {
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 0.5, 0.5, 1.5, 2 },
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 30,
                .lifetime = 30
            };
            ARR_PUSH(d->particles, new_part);
            ice_sparks(d, blt->pos, 0.05, rand() % 4 + 4);
            break;
        } case 9: {
            ARR_TRUNCATE(surr, 0);
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 1.5, 0.5, 0.5, 4 },
                .pos = { blt->pos[0], blt->pos[1], blt->pos[2] },
                .vel = { 0, 0, 0 },
                .ticks_left = 45,
                .lifetime = 45
            };
            ARR_PUSH(d->particles, new_part);
            red_sparks(d, blt->pos, 0.45, rand() % 4 + 12);
            float dist = vec3_dist(blt->pos, d->plr.pos);
            if(dist < 16) {
                dist = 1 - (dist / 16);
                pg_audio_play(&d->core->sounds[BORK_SND_PLAZMA_HIT], dist);
            }
            break;
        } default: break;
    }
}

void bork_bullet_move(struct bork_bullet* blt, struct bork_play_data* d)
{
    struct bork_map* map = &d->map;
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    vec3 surr_start, surr_end;
    vec3 surr_center;
    vec3_add(surr_center, blt->pos,
        (vec3){ blt->dir[0] * 0.5, blt->dir[1] * 0.5, blt->dir[2] * 0.5 });
    vec3_sub(surr_start, surr_center, (vec3){ 3, 3, 3 });
    vec3_add(surr_end, surr_center, (vec3){ 3, 3, 3 });
    bork_map_query_enemies(map, &surr, surr_start, surr_end);
    bork_map_query_entities(map, &surr, surr_start, surr_end);
    float curr_move = 0;
    float max_move = 0.01;
    float full_dist = vec3_len(blt->dir);
    vec3 max_move_dir = {};
    vec3_set_len(max_move_dir, blt->dir, max_move);
    vec3 new_pos = { blt->pos[0], blt->pos[1], blt->pos[2] };
    while(curr_move < full_dist) {
        if(blt->dist_moved > blt->range) {
            blt->flags |= BORK_BULLET_DEAD;
        }
        if(curr_move + max_move >= full_dist) {
            vec3_set_len(max_move_dir, blt->dir, full_dist - curr_move);
            curr_move = full_dist;
            blt->dist_moved += (full_dist - curr_move);
        } else {
            curr_move += max_move;
            blt->dist_moved += max_move;
        }
        vec3_add(new_pos, new_pos, max_move_dir);
        if(blt->flags & BORK_BULLET_HURTS_ENEMY) {
            /*  Hit the closest enemy  */
            float closest = 100;
            struct bork_entity* closest_ent = NULL;
            int i;
            bork_entity_t ent_id;
            struct bork_entity* ent;
            ARR_FOREACH(surr, ent_id, i) {
                ent = bork_entity_get(ent_id);
                if(!ent) continue;
                const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
                vec3 blt_to_ent;
                vec3_sub(blt_to_ent, blt->pos, ent->pos);
                float dist = vec3_len(blt_to_ent);
                if(ent->flags & BORK_ENTFLAG_ENTITY) dist *= 0.5;
                if(dist < prof->size[0] && dist < closest) {
                    closest = dist;
                    closest_ent = ent;
                }
            }
            if(closest_ent) {
                if(closest_ent->flags & BORK_ENTFLAG_ENEMY) closest_ent->aware_ticks = PLAY_SECONDS(4);
                closest_ent->HP -= blt->damage;
                blt->flags |= BORK_BULLET_DEAD;
                if(!(closest_ent->flags & BORK_ENTFLAG_STATIONARY)
                && closest_ent->freeze_ticks < 60 && blt->type != 8) {
                    vec3 knockback = {};
                    vec3_set_len(knockback, blt->dir, 0.1);
                    vec3_add(closest_ent->vel, closest_ent->vel, knockback);
                }
                vec3_set(blt->dir, 0, 0, 0);
                vec3_set(blt->light_color, 1, 1, 0.6);
                if(blt->type == BORK_ITEM_BULLETS_INC - BORK_ITEM_BULLETS
                || blt->type == BORK_ITEM_SHELLS_INC - BORK_ITEM_BULLETS) {
                    closest_ent->flags |= BORK_ENTFLAG_ON_FIRE;
                    closest_ent->fire_ticks = 360;
                } else if(blt->type == 8) {
                    closest_ent->freeze_ticks = MIN(closest_ent->freeze_ticks + 240, PLAY_SECONDS(10));
                }
                bullet_die(blt, d);
                return;
            }
        }
        if(blt->flags & BORK_BULLET_HURTS_PLAYER) {
            vec3 blt_to_plr;
            vec3_sub(blt_to_plr, blt->pos, map->plr->pos);
            vec3_abs(blt_to_plr, blt_to_plr);
            float dist = vec3_len(blt_to_plr);
            if(blt_to_plr[0] < 0.75 && blt_to_plr[1] < 0.75 && blt_to_plr[2] < 1) {
                pg_audio_play(&d->core->sounds[BORK_SND_HURT], 1);
                blt->flags |= BORK_BULLET_DEAD;
                hurt_player(d, blt->damage, 1);
                vec3_sub(blt->pos, new_pos, blt->dir);
                vec3_set(blt->dir, 0, 0, 0);
                vec3_set(blt->light_color, 2, 0.8, 0.8);
                bullet_die(blt, d);
                return;
            }
        }
        struct bork_map_object* hit_obj = NULL;
        if(bork_map_check_sphere(map, &hit_obj, new_pos, 0.1)) {
            blt->flags |= BORK_BULLET_DEAD;
            if(!hit_obj && (blt->type == BORK_ITEM_BULLETS_INC - BORK_ITEM_BULLETS
            || blt->type == BORK_ITEM_SHELLS_INC - BORK_ITEM_BULLETS)) {
                bork_map_create_fire(map, blt->pos, 360);
            } else if(hit_obj && hit_obj->type == BORK_MAP_GRATE) {
                hit_obj->dead = 1;
            }
            bullet_die(blt, d);
            return;
        }
    }
    vec3_dup(blt->pos, new_pos);
}

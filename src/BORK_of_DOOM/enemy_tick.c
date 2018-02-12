#include <stdio.h>
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

#define RANDF   ((float)rand() / RAND_MAX)

static void entity_emp(struct bork_play_data* d, struct bork_entity* ent)
{
    if(ent->emp_ticks <= 0) {
        ent->flags &= ~BORK_ENTFLAG_EMP;
        return;
    }
    if(ent->emp_ticks % 41 == 0) {
        create_smoke(d,
            (vec3){ ent->pos[0] + (RANDF - 0.5) * 0.5,
                    ent->pos[1] + (RANDF - 0.5) * 0.5,
                    ent->pos[2] + (RANDF - 0.5) * 0.5 },
            (vec3){}, 180);
    } else if(ent->emp_ticks % 17 == 0) {
        create_sparks(d, ent->pos, 0.075, rand() % 2 + 1);
    }
    --ent->emp_ticks;
}

static void robot_die(struct bork_play_data* d, struct bork_entity* ent)
{
    int num_parts = 0;
    switch(ent->type) {
    case BORK_ENEMY_TIN_CANINE: num_parts = 6; break;
    case BORK_ENEMY_SNOUT_DRONE: num_parts = 4; break;
    case BORK_ENEMY_BOTTWEILER: num_parts = 2; break;
    case BORK_ENEMY_FANG_BANGER: num_parts = 2; break;
    case BORK_ENEMY_GREAT_BANE: num_parts = 10; break;
    case BORK_ENEMY_LAIKA: num_parts = 10; break;
    default: return;
    }
    robot_explosion(d, ent->pos, num_parts);
    int fires = rand() % 2 + 2;
    int i;
    for(i = 0; i < fires; ++i) {
        vec3 off = { (RANDF - 0.5) * 0.5,
                     (RANDF - 0.5) * 0.5,
                     (RANDF) * 0.5 };
        struct bork_fire new_fire = {
            .flags = BORK_FIRE_MOVES,
            .audio_handle = -1,
            .pos = { ent->pos[0] + off[0], ent->pos[1] + off[1], ent->pos[2] + off[2] },
            .vel = { off[0] * 0.25, off[1] * 0.25, off[2] * 0.25 },
            .lifetime = PLAY_SECONDS(5) + PLAY_SECONDS(RANDF * 2) };
        ARR_PUSH(d->map.fires, new_fire);
    }
    ent->flags |= BORK_ENTFLAG_DEAD;
}

static const int path_dir[8][2] = {
    { -1, -1 }, { 0, -1 },  { 1, -1 },
    { -1, 0 },              { 1, 0 },
    { -1, 1 },  { 0, 1 },   { 1, 1 } };

static const uint8_t path_bits[8] = {
    (1 << PG_RIGHT) | (1 << PG_BACK), (1 << PG_BACK), (1 << PG_LEFT) | (1 << PG_BACK),
    (1 << PG_RIGHT),    (1 << PG_LEFT),
    (1 << PG_RIGHT) | (1 << PG_FRONT), (1 << PG_FRONT), (1 << PG_LEFT) | (1 << PG_FRONT) };

static int path_from_tile(struct bork_map* map, int x, int y, int z, int away)
{
    struct bork_tile* tile = bork_map_tile_ptri(map, x, y, z);
    int current = map->plr_dist[x][y][z];
    int i, closest = away ? 16 : 0, closest_i = 0;
    /*  Find which adjacent tile is both walkable, and
        closest to the player (using the distance field */
    for(i = 0; i < 8; ++i) {
        int dx = x + path_dir[i][0];
        int dy = y + path_dir[i][1];
        if((tile->travel_flags & path_bits[i]) != path_bits[i])
            continue;
        struct bork_tile* opp = bork_map_tile_ptri(map, dx, dy, z);
        if(opp && (opp->travel_flags & path_bits[7 - i]) != path_bits[7 - i])
            continue;
        int dist = map->plr_dist[dx][dy][z];
        if(!away && dist > current && dist > closest) {
            closest = dist;
            closest_i = i;
        } else if(away && dist < current
               && ((rand() % 2 == 0 && dist == closest) || dist < closest)) {
            closest = dist;
            closest_i = i;
        }
    }
    if(closest > 0) return closest_i;
    else return -1;
}

void bork_tick_snout_drone(struct bork_entity* ent, struct bork_play_data* d)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    static bork_entity_arr_t surr = {};
    bork_entity_t this_id = bork_entity_id(ent);
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    /*  Do basic physics    */
    bork_entity_move(ent, d);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
        if(ent_id == this_id) continue;
        surr_ent = bork_entity_get(ent_id);
        if(!surr_ent) continue;
        const struct bork_entity_profile* surr_prof = &BORK_ENT_PROFILES[surr_ent->type];
        vec3 push;
        vec3_sub(push, ent->pos, surr_ent->pos);
        float full = prof->size[0] + surr_prof->size[0];
        float dist = vec3_len(push);
        if(dist < full) {
            vec3_set_len(push, push, (full - dist) * 0.5);
            vec3_add(ent->vel, ent->vel, push);
        }
    }
    /*  Tick status effects */
    float move_speed = 0.015;
    if(ent->HP <= 0) {
        robot_die(d, ent);
        return;
    }
    if(ent->freeze_ticks) {
        float freeze_amount = 1 - MIN(ent->freeze_ticks / PLAY_SECONDS(2), 1);
        move_speed *= freeze_amount;
        --ent->freeze_ticks;
    }
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        ent->flags &= ~BORK_ENTFLAG_FLIES;
        entity_emp(d, ent);
        return;
    } else {
        ent->flags |= BORK_ENTFLAG_FLIES;
    }
    if(d->plr.HP <= 0) return;
    vec3 ent_head, plr_head;
    get_plr_pos_for_ai(d, plr_head);
    vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
    int vis = 0;
    if(ent->aware_ticks > PLAY_SECONDS(2.5)) vis = 1;
    else if((d->play_ticks + this_id) % 30 == 0) {
        vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = PLAY_SECONDS(5);
    }
    --ent->aware_ticks;
    if(vis && ent->aware_ticks >= 0 && !ent->freeze_ticks) {
        --ent->aware_ticks;
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, plr_head, ent->pos);
        float dist_xy = vec2_len(ent_to_plr);
        vec3_normalize(ent_to_plr, ent_to_plr);
        if(ent->counter[0] <= 0) ent->counter[0] = PLAY_SECONDS(2) + (RANDF * 60);
        else --ent->counter[0];
        if(ent->counter[0] == 0) {
            vec3 move = {};
            /*  Seek the goldilocks zone not too far and not to close   */
            if(dist_xy < 6) {
                vec2_scale(move, ent_to_plr, -1);
            } else if(dist_xy > 8) {
                vec2_dup(move, ent_to_plr);
            } else {
                if(rand() % 2 == 0) {
                    vec2_set(move, ent_to_plr[1], -ent_to_plr[0]);
                } else {
                    vec2_set(move, -ent_to_plr[1], ent_to_plr[0]);
                }
            }
            vec3_add(move, move,
                (vec3){ RANDF * 2 - 1, RANDF * 2 - 1, RANDF * 0.5 - 0.25 });
            vec3_normalize(move, move);
            vec3_dup(ent->dst_pos, move);
            vec3_add(move, move, ent->pos);
            bork_entity_look_at(ent, move);
        } else if(ent->counter[0] <= 60 && ent->counter[0] % 20 == 0 && dist_xy > 4) {
            bork_entity_look_at(ent, plr_head);
            struct bork_bullet new_bullet = { .type = 0,
                .flags = BORK_BULLET_HURTS_PLAYER,
                .damage = 3,
                .range = 120 };
            vec3_set_len(new_bullet.dir, ent_to_plr, 0.35);
            vec3_add(new_bullet.pos, ent->pos, (vec3){ 0, 0, -0.5 });
            vec3_add(new_bullet.pos, new_bullet.pos, new_bullet.dir);
            ARR_PUSH(d->bullets, new_bullet);
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .light = { 1.5, 1.5, 1.3, 2.0f },
                .vel = { 0, 0, 0 },
                .lifetime = 12,
                .ticks_left = 12,
                .frame_ticks = 3,
                .start_frame = 0, .end_frame = 5 };
            vec3_dup(new_part.pos, new_bullet.pos);
            vec3_add(new_part.pos, new_part.pos, new_bullet.dir);
            ARR_PUSH(d->particles, new_part);
            vec3 sound_pos;
            vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
            pg_audio_emit_once(&d->core->sounds[BORK_SND_PISTOL], 0.5, 16, sound_pos, 1);
        } else if(ent->counter[0] < 90) {
            bork_entity_turn_toward(ent, plr_head, 0.075);
        } else {
            vec3 move;
            vec3_scale(move, ent->dst_pos, 0.004);
            vec3_add(ent->vel, ent->vel, move);
        }
    }
}

void bork_tick_fang_banger(struct bork_entity* ent, struct bork_play_data* d)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    static bork_entity_arr_t surr = {};
    bork_entity_t this_id = bork_entity_id(ent);
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    /*  Do basic physics    */
    bork_entity_move(ent, d);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
        if(ent_id == this_id) continue;
        surr_ent = bork_entity_get(ent_id);
        if(!surr_ent) continue;
        const struct bork_entity_profile* surr_prof = &BORK_ENT_PROFILES[surr_ent->type];
        vec3 push;
        vec3_sub(push, ent->pos, surr_ent->pos);
        float full = prof->size[0] + surr_prof->size[0];
        float dist = vec3_len(push);
        if(dist < full) {
            vec3_set_len(push, push, (full - dist) * 0.5);
            vec3_add(ent->vel, ent->vel, push);
        }
    }
    /*  Tick status effects */
    if(ent->HP <= 0) {
        robot_die(d, ent);
        return;
    }
    float move_speed = 0.0125;
    if(ent->freeze_ticks) {
        float freeze_amount = 1 - MIN(ent->freeze_ticks / PLAY_SECONDS(2), 1);
        move_speed *= freeze_amount;
        --ent->freeze_ticks;
    }
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(d->plr.HP <= 0) return;
    vec3 ent_head, plr_head;
    get_plr_pos_for_ai(d, plr_head);
    vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.25 });
    int vis = 0;
    if(ent->aware_ticks > PLAY_SECONDS(2.5)) vis = 1;
    else if((d->play_ticks + this_id) % 30 == 0) {
        vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = PLAY_SECONDS(5);
    }
    if(ent->aware_ticks && !ent->freeze_ticks) {
        --ent->aware_ticks;
        if(ent->counter[0]) {
            --ent->counter[0];
            if(ent->counter[0] == 119) {
                vec3 sound_pos;
                vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
                pg_audio_emit_once(&d->core->sounds[BORK_SND_FASTBEEPS], 0.25, 16, sound_pos, 1);
            } else if(ent->counter[0] == 1) {
                ent->flags |= BORK_ENTFLAG_DEAD;
                game_explosion(d, ent->pos, 1.2);
            }
        } else {
            if(d->play_ticks % PLAY_SECONDS(0.75) == 0) {
                vec3 sound_pos;
                vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
                pg_audio_emit_once(&d->core->sounds[BORK_SND_SINGLEBEEP], 0.25, 16, sound_pos, 1);
            }
            /*  Movement/pathfinding    */
            /*  Calculate the next destination  */
            int x = floor(ent->pos[0] / 2);
            int y = floor(ent->pos[1] / 2);
            int z = floor(ent->pos[2] / 2);
            int plr_dist = d->map.plr_dist[x][y][z];
            if(plr_dist >= 15 && vec3_dist2(ent->pos, plr_head) < (1.5 * 1.5)) {
                ent->counter[0] = 120;
            } else if(plr_dist) {
                vec3 diff = {};
                vec2_sub(diff, ent->dst_pos, ent->pos);
                if(ent->path_ticks && vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                    /*  Just move to the last calculated destination    */
                    bork_entity_look_dir(ent, diff);
                    vec2_set_len(diff, diff, move_speed);
                    vec3_add(ent->vel, ent->vel, diff);
                    --ent->path_ticks;
                } else {
                    ent->path_ticks = PLAY_SECONDS(2.5);
                    /*  Calculate the next tile on the path */
                    int path = path_from_tile(&d->map, x, y, z, 0);
                    if(path >= 0) {
                        vec2 off = {
                            ((float)rand() / RAND_MAX - 0.5) * 1.5,
                            ((float)rand() / RAND_MAX - 0.5) * 1.5 };
                        vec3_set(ent->dst_pos,
                                 (x + path_dir[path][0]) * 2.0f + 1.0f + off[0],
                                 (y + path_dir[path][1]) * 2.0f + 1.0f + off[1],
                                 z * 2.0f + 1.0f);
                    }
                }
            }
        }
    }
}

void bork_tick_tin_canine(struct bork_entity* ent, struct bork_play_data* d)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[BORK_ENEMY_TIN_CANINE];
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    bork_entity_t this_id = bork_entity_id(ent);
    /*  Do basic physics    */
    bork_entity_move(ent, d);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
        if(ent_id == this_id) continue;
        surr_ent = bork_entity_get(ent_id);
        if(!surr_ent) continue;
        const struct bork_entity_profile* surr_prof = &BORK_ENT_PROFILES[surr_ent->type];
        vec3 push;
        vec3_sub(push, ent->pos, surr_ent->pos);
        float full = prof->size[0] + surr_prof->size[0];
        float dist = vec3_len(push);
        if(dist < full) {
            vec3_set_len(push, push, (full - dist) * 0.5);
            vec3_add(ent->vel, ent->vel, push);
        }
    }
    float move_speed = 0.005;
    if(ent->freeze_ticks) {
        float freeze_amount = 1 - MIN(ent->freeze_ticks / PLAY_SECONDS(2), 1);
        move_speed *= freeze_amount;
        --ent->freeze_ticks;
    }
    /*  Tick status effects */
    if(ent->HP <= 0) {
        robot_die(d, ent);
        return;
    }
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(d->plr.HP <= 0) return;
    /*  Real enemy tick */
    vec3 ent_head, plr_head;
    get_plr_pos_for_ai(d, plr_head);
    vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.4 });
    int vis = 0;
    if(ent->aware_ticks > PLAY_SECONDS(6)) vis = 1;
    else if((d->play_ticks + this_id) % 30 == 0) {
        vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = PLAY_SECONDS(12);
    }
    if(ent->aware_ticks && (ent->flags & BORK_ENTFLAG_GROUND)) {
        bork_entity_turn_toward(ent, plr_head, 0.05);
        --ent->aware_ticks;
        --ent->counter[0];
        if(ent->counter[0] <= 0) {
            ent->counter[0] = 180 + (RANDF * 90);
            ent->counter[1] = 0;
        } else if(!vis || ent->counter[0] > 100) {
            /*  Movement/pathfinding    */
            /*  Calculate the next destination  */
            int x = floor(ent->pos[0] / 2);
            int y = floor(ent->pos[1] / 2);
            int z = floor(ent->pos[2] / 2);
            int plr_dist = d->map.plr_dist[x][y][z];
            if(!vis || plr_dist < 12) {
                vec3 diff = {};
                vec2_sub(diff, ent->dst_pos, ent->pos);
                if(ent->path_ticks && vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                    /*  Just move to the last calculated destination    */
                    bork_entity_look_dir(ent, diff);
                    vec2_set_len(diff, diff, move_speed);
                    vec3_add(ent->vel, ent->vel, diff);
                    --ent->path_ticks;
                } else {
                    ent->path_ticks = PLAY_SECONDS(2.5);
                    /*  Calculate the next tile on the path */
                    int path = path_from_tile(&d->map, x, y, z, 0);
                    if(path >= 0) {
                        vec3_set(ent->dst_pos,
                                 (x + path_dir[path][0]) * 2.0f + 1.0f,
                                 (y + path_dir[path][1]) * 2.0f + 1.0f,
                                 z * 2.0f + 1.0f);
                    }
                }
            }
        } else if(vis && ent->counter[0] == 100 && !ent->freeze_ticks) {
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, plr_head, ent->pos);
            vec3_normalize(ent_to_plr, ent_to_plr);
            bork_entity_look_dir(ent, ent_to_plr);
            vec3 sound_pos;
            vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
            pg_audio_emit_once(&d->core->sounds[BORK_SND_CHARGE], 0.5, 16, sound_pos, 1);
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_EXPAND,
                .light = { 1.5, 0.5, 0.5, 3.0f },
                .vel = { 0, 0, 0 },
                .lifetime = 100,
                .ticks_left = 100 };
            vec3_add(new_part.pos, ent->pos, ent_to_plr);
            ARR_PUSH(d->particles, new_part);
            ent->counter[1] = 1;
        } else if(vis && !ent->freeze_ticks && ent->counter[0] == 1 && ent->counter[1] == 1) {
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, plr_head, ent->pos);
            struct bork_bullet new_bullet = { .type = 6,
                .flags = BORK_BULLET_HURTS_PLAYER,
                .damage = 5,
                .range = 120 };
            vec3_set_len(new_bullet.dir, ent_to_plr, 0.4);
            vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
            new_bullet.pos[2] -= 0.1;
            ARR_PUSH(d->bullets, new_bullet);
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .pos = { new_bullet.pos[0], new_bullet.pos[1], new_bullet.pos[2] },
                .light = { 1.5, 0.5, 0.5, 4.0f },
                .vel = { 0, 0, 0 },
                .lifetime = 16,
                .ticks_left = 16 };
            ARR_PUSH(d->particles, new_part);
            vec3 sound_pos;
            vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
            pg_audio_emit_once(&d->core->sounds[BORK_SND_PLAZGUN], 0.5, 16, sound_pos, 1);
        }

    }
}

void bork_tick_bottweiler(struct bork_entity* ent, struct bork_play_data* d)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    bork_entity_t this_id = bork_entity_id(ent);
    /*  Do basic physics    */
    bork_entity_move(ent, d);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
        if(ent_id == this_id) continue;
        surr_ent = bork_entity_get(ent_id);
        if(!surr_ent) continue;
        const struct bork_entity_profile* surr_prof = &BORK_ENT_PROFILES[surr_ent->type];
        vec3 push;
        vec3_sub(push, ent->pos, surr_ent->pos);
        float full = prof->size[0] + surr_prof->size[0];
        float dist = vec3_len(push);
        if(dist < full) {
            vec3_set_len(push, push, (full - dist) * 0.5);
            vec3_add(ent->vel, ent->vel, push);
        }
    }
    /*  Tick status effects */
    float move_speed = 0.01;
    if(ent->HP <= 0) {
        robot_die(d, ent);
        return;
    }
    if(ent->freeze_ticks) {
        float freeze_amount = 1 - MIN(ent->freeze_ticks / PLAY_SECONDS(2), 1);
        move_speed *= freeze_amount;
        --ent->freeze_ticks;
    }
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(d->plr.HP <= 0) return;
    vec3 ent_head, plr_head;
    get_plr_pos_for_ai(d, plr_head);
    vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
    int vis = 0;
    if(ent->aware_ticks > PLAY_SECONDS(2.5)) vis = 1;
    else if((d->play_ticks + this_id) % 30 == 0) {
        vis = bork_map_check_vis(&d->map, ent_head, plr_head) * 2;
        if(vis) ent->aware_ticks = PLAY_SECONDS(5);
    }
    if(ent->aware_ticks) {
        --ent->aware_ticks;
        /*  Movement/pathfinding    */
        /*  Calculate the next destination  */
        int x = floor(ent->pos[0] / 2);
        int y = floor(ent->pos[1] / 2);
        int z = floor(ent->pos[2] / 2);
        int plr_dist = d->map.plr_dist[x][y][z];
        float dist = vec3_dist(ent->pos, d->plr.pos);
        /*  Attack code */
        if(ent->counter[0] <= 0) {
            ent->counter[0] = 240 + RANDF * 45;
        } else --ent->counter[0];
        if(vis && ent->counter[0] == 120 && !ent->freeze_ticks) {
            float dist = vec3_dist(ent->pos, d->plr.pos);
            if(dist < 2) {
                vec3 ent_to_plr;
                vec3_sub(ent_to_plr, d->plr.pos, ent->pos);
                vec3_normalize(ent_to_plr, ent_to_plr);
                vec3_add(ent_to_plr, ent_to_plr, ent->pos);
                vec3 sound_pos;
                vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
                pg_audio_emit_once(&d->core->sounds[BORK_SND_BUZZ], 4, 12, sound_pos, 1);
                struct bork_particle new_part = {
                    .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                    .pos = { ent_to_plr[0], ent_to_plr[1], ent_to_plr[2] },
                    .light = { 1.0, 1.0, 1.5, 4 },
                    .vel = { 0, 0, 0 },
                    .lifetime = 30,
                    .ticks_left = 30,
                    .frame_ticks = 6,
                    .current_frame = 40,
                    .start_frame = 40, .end_frame = 44,
                };
                ARR_PUSH(d->particles, new_part);
                if(vis == 1) {
                    vis = bork_map_check_vis(&d->map, ent_head, plr_head);
                }
                float decoy_dist = vec3_dist2(d->plr.pos, plr_head);
                if(vis) {
                    pg_audio_play_ch(&d->core->sounds[BORK_SND_HURT], 1, 1);
                    if(decoy_dist < 1) {
                        hurt_player(d, 15, 1);
                        vec3 push;
                        vec3_sub(push, plr_head, ent->pos);
                        vec3_set_len(push, push, 0.075);
                        vec3_add(d->plr.vel, d->plr.vel, push);
                    }
                }
            } else ent->counter[0] = 180;
        } else if(plr_dist && dist > 2) {
            vec3 diff = {};
            vec2_sub(diff, ent->dst_pos, ent->pos);
            if(ent->path_ticks && vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                /*  Just move to the last calculated destination    */
                bork_entity_look_dir(ent, diff);
                vec2_set_len(diff, diff, move_speed);
                vec3_add(ent->vel, ent->vel, diff);
                --ent->path_ticks;
            } else {
                ent->path_ticks = 30;
                /*  Calculate the next tile on the path */
                int path = path_from_tile(&d->map, x, y, z, ent->counter[0] < 120);
                if(path >= 0) {
                    vec2 off = {
                        ((float)rand() / RAND_MAX - 0.5) * 1.5,
                        ((float)rand() / RAND_MAX - 0.5) * 1.5 };
                    vec3_set(ent->dst_pos,
                             (x + path_dir[path][0]) * 2.0f + 1.0f + off[0],
                             (y + path_dir[path][1]) * 2.0f + 1.0f + off[1],
                             z * 2.0f + 1.0f);
                }
            }
        }
    }
}

void bork_tick_great_bane(struct bork_entity* ent, struct bork_play_data* d)
{
    bork_entity_t this_id = bork_entity_id(ent);
    /*  Do basic physics    */
    bork_entity_move(ent, d);
    /*  Tick status effects */
    if(ent->HP <= 0) {
        robot_die(d, ent);
        return;
    }
    if(d->plr.HP <= 0) return;
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(ent->freeze_ticks) --ent->freeze_ticks;
    vec3 ent_head, plr_head;
    get_plr_pos_for_ai(d, plr_head);
    vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
    plr_head[2] -= 0.2;
    int vis = 0;
    if(ent->aware_ticks > PLAY_SECONDS(6)) vis = 1;
    else if((d->play_ticks + this_id) % 30 == 0) {
        vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = PLAY_SECONDS(12);
    }
    if(ent->aware_ticks && !ent->freeze_ticks) {
        --ent->aware_ticks;
        vec3 ent_to_plr;
        vec3_sub(ent_to_plr, plr_head, ent->pos);
        if(ent->counter[0] - d->play_ticks < 0) {
            ent->counter[0] = d->play_ticks + PLAY_SECONDS(2) + rand() % 60;
        } else if(ent->counter[0] - d->play_ticks <= 60 && ((ent->counter[0] - d->play_ticks) % 15 == 0)) {
            float angle = M_PI + atan2f(plr_head[0] - ent->pos[0], -(plr_head[1] - ent->pos[1]));
            float diff = ent->dir[0] - angle;
            diff = fabs(diff);
            if(diff > M_PI * 0.5) return;
            struct bork_bullet new_bullet = { .type = 9,
                .flags = BORK_BULLET_HURTS_PLAYER,
                .damage = 30,
                .range = 120 };
            vec2 sph = {
                ent->dir[0] + M_PI * 0.5 + (RANDF * 0.2 - 0.1),
                M_PI - atan2f(vec2_len(ent_to_plr), ent->pos[2] - plr_head[2]) };
            spherical_to_cartesian(new_bullet.dir, sph);
            vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
            vec3_set_len(new_bullet.dir, new_bullet.dir, 0.25);
            ARR_PUSH(d->bullets, new_bullet);
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
                .pos = { new_bullet.pos[0], new_bullet.pos[1], new_bullet.pos[2] },
                .light = { 1.5, 0.5, 0.5, 6.0f },
                .vel = { 0, 0, 0 },
                .lifetime = 16,
                .ticks_left = 16 };
            ARR_PUSH(d->particles, new_part);
            vec3 sound_pos;
            vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
            pg_audio_emit_once(&d->core->sounds[BORK_SND_PLAZGUN], 1, 32, sound_pos, 1);
        } else {
            bork_entity_turn_toward(ent, plr_head, 0.006);
        }
    }
}

void bork_tick_laika(struct bork_entity* ent, struct bork_play_data* d)
{
    bork_entity_t this_id = bork_entity_id(ent);
    bork_entity_move(ent, d);
    if(ent->HP <= 0) {
        d->killed_laika = 1;
        robot_die(d, ent);
        return;
    }
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->freeze_ticks) --ent->freeze_ticks;
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    vec3 ent_head, plr_head;
    get_plr_pos_for_ai(d, plr_head);
    vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
    plr_head[2] -= 0.2;
    vec3 ent_to_plr;
    vec3_sub(ent_to_plr, plr_head, ent->pos);
    int vis = 0;
    if(ent->aware_ticks > PLAY_SECONDS(6)) vis = 1;
    else if((d->play_ticks + this_id) % 30 == 0) {
        vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = PLAY_SECONDS(12);
    }
    static const vec2 center_spire = { 31, 33 };
    if(!ent->aware_ticks || ent->freeze_ticks) return;
    if(ent->counter[0] == 0) {
        if(ent->counter[1] < 2 && rand() % 4 == 0) {
            ent->counter[0] = 600;
            ent->counter[1] = 2;
            vec2 ctr_to_plr, ctr_to_laika;
            vec2_sub(ctr_to_plr, plr_head, center_spire);
            vec2_sub(ctr_to_laika, ent->pos, center_spire);
            float plr_angle = M_PI + atan2f(ctr_to_plr[0], ctr_to_plr[1]);
            float laika_angle = M_PI + atan2f(ctr_to_laika[0], ctr_to_laika[1]);
            float angle_diff = plr_angle - laika_angle;
            float t = FMOD(angle_diff + M_PI, M_PI * 2) - M_PI;
            ent->counter[2] = -SGN(t);
        } else {
            ent->counter[1] = !ent->counter[1];
            ent->counter[0] = 300;
        }
    } else --ent->counter[0];
    if(ent->counter[1] == 2) {
        float t = (float)ent->counter[2] * 0.3;
        vec2 ctr_to_laika;
        vec2_sub(ctr_to_laika, ent->pos, center_spire);
        float laika_angle = M_PI + atan2f(ctr_to_laika[0], ctr_to_laika[1]);
        float target_angle = laika_angle + t;
        vec3 target_pos = { sin(target_angle) * -4, cos(target_angle) * -4, ent->pos[2] };
        vec2_add(target_pos, target_pos, center_spire);
        vec2_dup(ent->dst_pos, target_pos);
        bork_entity_look_at(ent, target_pos);
        vec2 move = {};
        vec2_sub(move, ent->dst_pos, ent->pos);
        vec2_set_len(move, move, 0.02);
        vec2_dup(ent->vel, move);
    } else if(ent->counter[0] > 120) {
        if((d->play_ticks + this_id) % 20 == 0) {
            vec2 center_spire = { 31, 33 };
            vec2 ctr_to_plr, ctr_to_laika;
            vec2_sub(ctr_to_plr, d->plr.pos, center_spire);
            vec2_sub(ctr_to_laika, ent->pos, center_spire);
            float plr_angle = M_PI + atan2f(ctr_to_plr[0], ctr_to_plr[1]);
            float laika_angle = M_PI + atan2f(ctr_to_laika[0], ctr_to_laika[1]);
            float angle_diff = plr_angle - laika_angle;
            float t = FMOD(angle_diff + M_PI, M_PI * 2) - M_PI;
            if(fabs(t) > 1.5) {
                t = SGN(t) * 0.3;
                float target_angle = laika_angle + t;
                vec3 target_pos = { sin(target_angle) * -4, cos(target_angle) * -4, ent->pos[2] };
                vec2_add(target_pos, target_pos, center_spire);
                vec2_dup(ent->dst_pos, target_pos);
                bork_entity_look_at(ent, target_pos);
            } else {
                vec3_dup(ent->dst_pos, ent->pos);
            }
        }
        vec2 move = {};
        vec2_sub(move, ent->dst_pos, ent->pos);
        vec2_set_len(move, move, 0.02);
        vec2_dup(ent->vel, move);
    } else if(ent->counter[0] == 120 && ent->counter[1] == 0) {
        bork_entity_look_at(ent, plr_head);
        vec3 sound_pos;
        vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
        pg_audio_emit_once(&d->core->sounds[BORK_SND_CHARGE], 0.5, 32, sound_pos, 1);
        struct bork_particle new_part = {
            .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_EXPAND,
            .light = { 1.5, 0.5, 0.5, 8.0f },
            .vel = { 0, 0, 0 },
            .lifetime = 60,
            .ticks_left = 60 };
        vec3 part_offset = {};
        vec3_set_len(part_offset, ent_to_plr, 2);
        vec3_add(new_part.pos, ent->pos, part_offset);
        ARR_PUSH(d->particles, new_part);
    } else if(ent->counter[0] < 60 && ent->counter[0] % 20 == 0 && ent->counter[1] == 0) {
        bork_entity_look_at(ent, plr_head);
        int i;
        for(i = 0; i < 3; ++i) {
            struct bork_bullet new_bullet = { .type = (i == 1) ? 9 : 6,
                .flags = BORK_BULLET_HURTS_PLAYER,
                .damage = i == 1 ? 30 : 10,
                .range = 120 };
            vec2 sph = {
                ent->dir[0] + M_PI * 0.5,
                M_PI - atan2f(vec2_len(ent_to_plr), ent->pos[2] - plr_head[2]) };
            spherical_to_cartesian(new_bullet.dir, sph);
            float side = (M_PI * 0.5) * SGN(i - 1);
            vec3 blt_off = { cos(sph[0] + side) * 0.5, sin(sph[0] + side) * 0.5, -0.4 };
            if(i != 1) vec3_add(new_bullet.pos, ent->pos, blt_off);
            else vec3_dup(new_bullet.pos, ent->pos);
            vec3_add(new_bullet.pos, new_bullet.pos, new_bullet.dir);
            vec3_set_len(new_bullet.dir, new_bullet.dir, 0.25);
            ARR_PUSH(d->bullets, new_bullet);
        }
        vec3 sound_pos;
        vec3_mul(sound_pos, ent->pos, (vec3){ 1, 1, 2 });
        pg_audio_emit_once(&d->core->sounds[BORK_SND_PLAZGUN], 0.5, 32, sound_pos, 1);
        struct bork_particle new_part = {
            .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
            .light = { 1.5, 0.5, 0.5, 4.0f },
            .vel = { 0, 0, 0 },
            .lifetime = 16,
            .ticks_left = 16 };
        vec3 part_offset = {};
        vec3_set_len(part_offset, ent_to_plr, 1);
        vec3_add(new_part.pos, ent->pos, part_offset);
        ARR_PUSH(d->particles, new_part);
        red_sparks(d, new_part.pos, 0.25, rand() % 4 + 4);
    } else if(ent->counter[0] <= 100 && ent->counter[0] % 10 == 0 && ent->counter[1] == 1) {
        if(ent->counter[0] % 20 == 0) bork_entity_look_at(ent, plr_head);
        struct bork_bullet new_bullet = { .type = 0,
            .flags = BORK_BULLET_HURTS_PLAYER,
            .damage = 5,
            .range = 120 };
        vec2 sph = {
            ent->dir[0] + M_PI * 0.5,
            M_PI - atan2f(vec2_len(ent_to_plr), ent->pos[2] - plr_head[2]) };
        float side = (ent->counter[0] % 20 == 0) ? M_PI * 0.5 : M_PI * -0.5;
        vec3 blt_off = { cos(sph[0] + side) * 0.8, sin(sph[0] + side) * 0.8, -0.2 };
        spherical_to_cartesian(new_bullet.dir, sph);
        vec3_set_len(new_bullet.dir, new_bullet.dir, 0.5);
        vec3_add(new_bullet.pos, ent->pos, blt_off);
        vec3_add(new_bullet.pos, new_bullet.pos, new_bullet.dir);
        ARR_PUSH(d->bullets, new_bullet);
        struct bork_particle new_part = {
            .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
            .light = { 1.5, 1.5, 1.3, 2.0f },
            .vel = { 0, 0, 0 },
            .lifetime = 12,
            .ticks_left = 12,
            .frame_ticks = 3,
            .start_frame = 0, .end_frame = 5 };
        vec3_dup(new_part.pos, new_bullet.pos);
        ARR_PUSH(d->particles, new_part);
    }
}

void bork_tick_test_enemy(struct bork_entity* ent, struct bork_play_data* d)
{
    bork_entity_move(ent, d);
    if(ent->counter[0] - d->play_ticks <= 0) {
        ent->counter[0] = d->play_ticks + PLAY_SECONDS(4);
    } else if(ent->counter[0] - d->play_ticks < PLAY_SECONDS(1)) {
        bork_entity_look_at(ent, d->plr.pos);
    } else {
        bork_entity_look_dir(ent, (vec3){ 0, 1 });
        ent->vel[1] = 0.001;
    }
}



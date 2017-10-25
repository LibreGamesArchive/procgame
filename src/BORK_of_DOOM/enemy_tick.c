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
        vec3 spark_pos = {
            ent->pos[0] + (float)rand() / RAND_MAX - 0.5,
            ent->pos[1] + (float)rand() / RAND_MAX - 0.5,
            ent->pos[2] + (float)rand() / RAND_MAX - 0.5 };
        create_spark(d, spark_pos);
    }
    --ent->emp_ticks;
}

static void robot_dying(struct bork_play_data* d, struct bork_entity* ent)
{
    --ent->dead_ticks;
    if(ent->dead_ticks % 30 == 0) {
        vec3 spark_pos = {
            ent->pos[0] + (float)rand() / RAND_MAX - 0.5,
            ent->pos[1] + (float)rand() / RAND_MAX - 0.5,
            ent->pos[2] + (float)rand() / RAND_MAX - 0.5 };
        create_spark(d, spark_pos);
    } else if(ent->dead_ticks <= 0) {
        bork_entity_t ent_id = bork_entity_id(ent);
        robot_explosion(d, ent->pos);
        ent = bork_entity_get(ent_id);
        ent->flags |= BORK_ENTFLAG_DEAD;
    }
}

static const int path_dir[8][2] = {
    { -1, -1 }, { 0, -1 },  { 1, -1 },
    { -1, 0 },              { 1, 0 },
    { -1, 1 },  { 0, 1 },   { 1, 1 } };

static const uint8_t path_bits[8] = {
    (1 << PG_RIGHT) | (1 << PG_BACK), (1 << PG_BACK), (1 << PG_LEFT) | (1 << PG_BACK),
    (1 << PG_RIGHT),    (1 << PG_LEFT),
    (1 << PG_RIGHT) | (1 << PG_FRONT), (1 << PG_FRONT), (1 << PG_LEFT) | (1 << PG_FRONT) };

static int path_from_tile(struct bork_map* map, int x, int y, int z)
{
    struct bork_tile* tile = bork_map_tile_ptri(map, x, y, z);
    int current = map->plr_dist[x][y][z];
    int i, closest = 0, closest_i = 0;
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
        if(dist > current && dist > closest) {
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
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    /*  Do basic physics    */
    bork_entity_move(ent, &d->map);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
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
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        ent->flags &= ~BORK_ENTFLAG_FLIES;
        entity_emp(d, ent);
        return;
    } else {
        ent->flags |= BORK_ENTFLAG_FLIES;
    }
    if(ent->flags & BORK_ENTFLAG_DYING) {
        ent->flags &= ~BORK_ENTFLAG_FLIES;
        robot_dying(d, ent);
    } else if(ent->HP <= 0) {
        ent->flags |= BORK_ENTFLAG_DYING;
        ent->flags &= ~BORK_ENTFLAG_FLIES;
        ent->dead_ticks = PLAY_SECONDS(2.5);
    } else {
        vec3 ent_head, plr_head;
        get_plr_pos_for_ai(d, plr_head);
        vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
        int vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) {
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, plr_head, ent->pos);
            vec3_normalize(ent_to_plr, ent_to_plr);
            if(ent->counter[1] == 0) {
                vec3_set_len(ent->vel, ent->dst_pos, 0.015);
                if(ent->counter[0] - d->ticks <= 0) {
                    bork_entity_look_dir(ent, ent_to_plr);
                    ent->counter[1] = 1;
                    ent->counter[0] = d->ticks + PLAY_SECONDS(2);
                }
            } else if(ent->counter[1] == 1) {
                if(ent->counter[0] - d->ticks > 60) {
                    vec3_set_len(ent->vel, ent_to_plr, 0.015);
                    bork_entity_look_dir(ent, ent_to_plr);
                } else if(ent->counter[0] - d->ticks == 60) {
                    vec3_set(ent->vel, 0, 0, 0);
                } else if(ent->counter[0] - d->ticks == 30) {
                    struct bork_bullet new_bullet = { .type = 6,
                        .flags = BORK_BULLET_HURTS_PLAYER,
                        .damage = 10 };
                    vec3_set_len(new_bullet.dir, ent_to_plr, 0.4);
                    vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
                    new_bullet.pos[2] -= 0.3;
                    ARR_PUSH(d->bullets, new_bullet);
                } else if(ent->counter[0] - d->ticks <= 0) {
                    ent->counter[1] = 0;
                    vec3 off = {
                        ((float)rand() / RAND_MAX - 0.5),
                        ((float)rand() / RAND_MAX - 0.5),
                        ((float)rand() / RAND_MAX - 0.5) * 0.5 };
                    struct bork_tile* tile = bork_map_tile_ptr(&d->map, ent->pos);
                    if(tile->travel_flags & (1 << 6)) off[2] = 1;
                    vec3_dup(ent->dst_pos, off);
                    ent->counter[0] = d->ticks + PLAY_SECONDS(1.5) + PLAY_SECONDS(RANDF);
                    bork_entity_look_dir(ent, off);
                }
            }
        } else {
            vec3_set(ent->vel, 0, 0, 0);
        }
    }
}

void bork_tick_fang_banger(struct bork_entity* ent, struct bork_play_data* d)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    /*  Do basic physics    */
    bork_entity_move(ent, &d->map);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
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
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(ent->flags & BORK_ENTFLAG_DYING) {
        robot_dying(d, ent);
    } else if(ent->HP <= 0) {
        ent->flags |= BORK_ENTFLAG_DYING;
        ent->dead_ticks = PLAY_SECONDS(0.5);
    } else {
        vec3 ent_head, plr_head;
        get_plr_pos_for_ai(d, plr_head);
        vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
        int vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = 1200;
        else if(ent->aware_ticks) --ent->aware_ticks;
        if(ent->aware_ticks) {
            /*  Movement/pathfinding    */
            /*  Calculate the next destination  */
            int x = floor(ent->pos[0] / 2);
            int y = floor(ent->pos[1] / 2);
            int z = floor(ent->pos[2] / 2);
            int plr_dist = d->map.plr_dist[x][y][z];
            if(plr_dist >= 15 && vec3_dist(ent->pos, plr_head) < 2.5) {
                ent->flags |= BORK_ENTFLAG_DEAD;
                game_explosion(d, ent->pos, 0.5);
            } else if(plr_dist) {
                vec3 diff = {};
                vec2_sub(diff, ent->dst_pos, ent->pos);
                if(ent->path_ticks && vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                    /*  Just move to the last calculated destination    */
                    bork_entity_look_dir(ent, diff);
                    vec2_set_len(diff, diff, 0.0125);
                    vec3_add(ent->vel, ent->vel, diff);
                    --ent->path_ticks;
                } else {
                    ent->path_ticks = PLAY_SECONDS(2.5);
                    /*  Calculate the next tile on the path */
                    int path = path_from_tile(&d->map, x, y, z);
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
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    vec3 start, end;
    vec3_sub(start, ent->pos, (vec3){ 2, 2, 2 });
    vec3_add(end, ent->pos, (vec3){ 2, 2, 2 });
    bork_map_query_enemies(&d->map, &surr, start, end);
    /*  Do basic physics    */
    bork_entity_move(ent, &d->map);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
        surr_ent = bork_entity_get(ent_id);
        if(!surr_ent) continue;
        vec3 push;
        vec3_sub(push, ent->pos, surr_ent->pos);
        float dist = vec3_len(push);
        if(dist < 1.0f) {
            vec3_set_len(push, push, 1.0f - dist);
            vec3_add(ent->vel, ent->vel, push);
        }
    }
    /*  Tick status effects */
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    /*  Real enemy tick */
    if(ent->flags & BORK_ENTFLAG_DYING) {
        robot_dying(d, ent);
    } else if(ent->HP <= 0) {
        ent->flags |= BORK_ENTFLAG_DYING;
        ent->dead_ticks = PLAY_SECONDS(1.5);
    } else {
        vec3 ent_head, plr_head;
        get_plr_pos_for_ai(d, plr_head);
        vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
        int vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = 1200;
        else if(ent->aware_ticks) --ent->aware_ticks;
        if(ent->aware_ticks && (ent->flags & BORK_ENTFLAG_GROUND)) {
            if(ent->counter[0] - d->ticks <= 0) {
                ent->counter[0] = d->ticks + 180 + (RANDF * 90);
            } else if(!vis || ent->counter[0] - d->ticks > 60) {
                /*  Movement/pathfinding    */
                /*  Calculate the next destination  */
                int x = floor(ent->pos[0] / 2);
                int y = floor(ent->pos[1] / 2);
                int z = floor(ent->pos[2] / 2);
                int plr_dist = d->map.plr_dist[x][y][z];
                if(plr_dist) {
                    vec3 diff = {};
                    vec2_sub(diff, ent->dst_pos, ent->pos);
                    if(ent->path_ticks && vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                        /*  Just move to the last calculated destination    */
                        bork_entity_look_dir(ent, diff);
                        vec2_set_len(diff, diff, 0.005);
                        vec3_add(ent->vel, ent->vel, diff);
                        --ent->path_ticks;
                    } else {
                        ent->path_ticks = PLAY_SECONDS(2.5);
                        /*  Calculate the next tile on the path */
                        int path = path_from_tile(&d->map, x, y, z);
                        if(path >= 0) {
                            vec3_set(ent->dst_pos,
                                     (x + path_dir[path][0]) * 2.0f + 1.0f,
                                     (y + path_dir[path][1]) * 2.0f + 1.0f,
                                     z * 2.0f + 1.0f);
                        }
                    }
                }
            } else if(vis) {
                vec3 ent_to_plr;
                vec3_sub(ent_to_plr, plr_head, ent->pos);
                bork_entity_look_dir(ent, ent_to_plr);
                if(ent->counter[0] - d->ticks == 30) {
                    struct bork_bullet new_bullet = { .type = 6,
                        .flags = BORK_BULLET_HURTS_PLAYER,
                        .damage = 10 };
                    vec3_set_len(new_bullet.dir, ent_to_plr, 0.4);
                    vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
                    new_bullet.pos[2] -= 0.1;
                    ARR_PUSH(d->bullets, new_bullet);
                }
            }
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
    /*  Do basic physics    */
    bork_entity_move(ent, &d->map);
    /*  Physics against other enemies   */
    int i;
    struct bork_entity* surr_ent;
    bork_entity_t ent_id;
    ARR_FOREACH(surr, ent_id, i) {
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
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(ent->flags & BORK_ENTFLAG_DYING) {
        robot_dying(d, ent);
    } else if(ent->HP <= 0) {
        ent->flags |= BORK_ENTFLAG_DYING;
        ent->dead_ticks = PLAY_SECONDS(0.5);
    } else {
        vec3 ent_head, plr_head;
        get_plr_pos_for_ai(d, plr_head);
        vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
        int vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = 1200;
        else if(ent->aware_ticks) --ent->aware_ticks;
        if(ent->aware_ticks) {
            /*  Movement/pathfinding    */
            /*  Calculate the next destination  */
            int x = floor(ent->pos[0] / 2);
            int y = floor(ent->pos[1] / 2);
            int z = floor(ent->pos[2] / 2);
            int plr_dist = d->map.plr_dist[x][y][z];
            if(ent->counter[1]) {
                --ent->counter[1];
                if(vis && ent->counter[1] == 60 && vec3_dist(ent->pos, d->plr.pos) < 2) {
                    d->plr.HP -= 15;
                    vec3 push;
                    vec3_sub(push, d->plr.pos, ent->pos);
                    vec3_set_len(push, push, 0.15);
                    vec3_add(d->plr.vel, d->plr.vel, push);
                }
            } else if(vis && vec3_dist(ent->pos, d->plr.pos) < 2) {
                ent->counter[1] = PLAY_SECONDS(1);
            } else if(plr_dist) {
                vec3 diff = {};
                vec2_sub(diff, ent->dst_pos, ent->pos);
                if(ent->path_ticks && vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                    /*  Just move to the last calculated destination    */
                    bork_entity_look_dir(ent, diff);
                    vec2_set_len(diff, diff, 0.01);
                    vec3_add(ent->vel, ent->vel, diff);
                    --ent->path_ticks;
                } else {
                    ent->path_ticks = PLAY_SECONDS(2.5);
                    /*  Calculate the next tile on the path */
                    int path = path_from_tile(&d->map, x, y, z);
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

void bork_tick_great_bane(struct bork_entity* ent, struct bork_play_data* d)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    /*  Do basic physics    */
    bork_entity_move(ent, &d->map);
    /*  Tick status effects */
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_EMP) {
        entity_emp(d, ent);
        return;
    }
    if(ent->flags & BORK_ENTFLAG_DYING) {
        ent->flags &= ~BORK_ENTFLAG_FLIES;
        robot_dying(d, ent);
    } else if(ent->HP <= 0) {
        ent->flags |= BORK_ENTFLAG_DYING;
        ent->flags &= ~BORK_ENTFLAG_FLIES;
        ent->dead_ticks = PLAY_SECONDS(3);
    } else {
        vec3 plr_head;
        get_plr_pos_for_ai(d, plr_head);
        plr_head[2] -= 0.2;
        int vis = bork_map_check_vis(&d->map, ent->pos, plr_head);
        if(vis) {
            vec3 ent_to_plr;
            vec3_sub(ent_to_plr, plr_head, ent->pos);
            if(ent->counter[0] - d->ticks <= 0) {
                ent->counter[0] = d->ticks + PLAY_SECONDS(0.5);
                struct bork_bullet new_bullet = { .type = 9,
                    .flags = BORK_BULLET_HURTS_PLAYER,
                    .damage = 10 };
                vec2 sph = {
                    ent->dir[0] + M_PI * 0.5,
                    M_PI - atan2f(vec2_len(ent_to_plr), ent->pos[2] - plr_head[2]) };
                spherical_to_cartesian(new_bullet.dir, sph);
                vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
                vec3_set_len(new_bullet.dir, new_bullet.dir, 0.1);
                ARR_PUSH(d->bullets, new_bullet);
                sph[0] -= M_PI * 0.2;
                spherical_to_cartesian(new_bullet.dir, sph);
                vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
                vec3_set_len(new_bullet.dir, new_bullet.dir, 0.1);
                ARR_PUSH(d->bullets, new_bullet);
                sph[0] += M_PI * 0.4;
                spherical_to_cartesian(new_bullet.dir, sph);
                vec3_add(new_bullet.pos, ent->pos, new_bullet.dir);
                vec3_set_len(new_bullet.dir, new_bullet.dir, 0.1);
                ARR_PUSH(d->bullets, new_bullet);
            } else {
                bork_entity_turn_toward(ent, plr_head, 0.004);
                //bork_entity_look_dir(ent, ent_to_plr);
            }
        }
    }
}

void bork_tick_test_enemy(struct bork_entity* ent, struct bork_play_data* d)
{
    bork_entity_move(ent, &d->map);
    if(ent->counter[0] - d->ticks <= 0) {
        ent->counter[0] = d->ticks + PLAY_SECONDS(4);
    } else if(ent->counter[0] - d->ticks < PLAY_SECONDS(1)) {
        bork_entity_look_at(ent, d->plr.pos);
    } else {
        bork_entity_look_dir(ent, (vec3){ 0, 1 });
        ent->vel[1] = 0.001;
    }
}



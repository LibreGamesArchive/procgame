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
#include "game_states.h"

#define RANDF   ((float)rand() / RAND_MAX)

static void entity_on_fire(struct bork_play_data* d, struct bork_entity* ent)
{
    if(ent->counter[2] % 41 == 0) {
        create_smoke(d,
            (vec3){ ent->pos[0] + (RANDF - 0.5) * 0.5,
                    ent->pos[1] + (RANDF - 0.5) * 0.5,
                    ent->pos[2] + (RANDF - 0.5) * 0.5 },
            (vec3){}, 180);
    } else if(ent->counter[2] % 31 == 0) {
        struct bork_particle new_part = {
            .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT | BORK_PARTICLE_DECELERATE,
            .pos = { ent->pos[0] + (RANDF - 0.5) * 0.5,
                     ent->pos[1] + (RANDF - 0.5) * 0.5,
                     ent->pos[2] + (RANDF - 0.5) * 0.5 },
            .vel = { 0, 0, -0.005 },
            .ticks_left = 120,
            .frame_ticks = 0,
            .current_frame = 24 + rand() % 4,
        };
        ARR_PUSH(d->particles, new_part);
    }
    --ent->counter[2];
    if(ent->counter[2] == 0) ent->flags &= ~BORK_ENTFLAG_ON_FIRE;
}

static void robot_dying(struct bork_play_data* d, struct bork_entity* ent)
{
    --ent->dead_ticks;
    if(ent->dead_ticks > 60 && ent->dead_ticks % 30 == 0) {
        vec3 spark_pos = {
            ent->pos[0] + (float)rand() / RAND_MAX - 0.5,
            ent->pos[1] + (float)rand() / RAND_MAX - 0.5,
            ent->pos[2] + (float)rand() / RAND_MAX - 0.5 };
        create_spark(d, spark_pos);
    } else if(ent->dead_ticks == 60) {
        robot_explosion(d, ent->pos);
        return;
    } else if(ent->dead_ticks <= 0) {
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
void tin_canine_tick(struct bork_play_data* d, struct bork_entity* ent)
{
    if(ent->flags & BORK_ENTFLAG_ON_FIRE) entity_on_fire(d, ent);
    if(ent->flags & BORK_ENTFLAG_DYING) robot_dying(d, ent);
    else if(ent->HP <= 0) {
        ent->flags |= BORK_ENTFLAG_DYING;
        ent->dead_ticks = 180;
        return;
    } else {
        vec3 ent_head, plr_head;
        vec3_add(ent_head, ent->pos, (vec3){ 0, 0, 0.5 });
        vec3_add(plr_head, d->plr.pos, (vec3){ 0, 0, 0.5 });
        int vis = bork_map_check_vis(&d->map, ent_head, plr_head);
        if(vis) ent->aware_ticks = 1200;
        else if(ent->aware_ticks) --ent->aware_ticks;
        if(ent->aware_ticks) {
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
                    if(vec2_len(diff) > 0.5 && vec2_len(diff) < 16) {
                        /*  Just move to the last calculated destination    */
                        float angle = atan2f(diff[0], diff[1]) + M_PI;
                        ent->dir[0] = (M_PI * 2 - angle) - M_PI * 0.5;
                        ent->dir[0] = FMOD(ent->dir[0], M_PI * 2);
                        vec2_set_len(diff, diff, 0.005);
                        vec3_add(ent->vel, ent->vel, diff);
                    } else {
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
                vec3_sub(ent_to_plr, plr_head, ent_head);
                float angle = atan2f(ent_to_plr[0], ent_to_plr[1]) + M_PI;
                ent->dir[0] = (M_PI * 2 - angle) - M_PI * 0.5;
                if(ent->counter[0] - d->ticks == 30) {
                    struct bork_bullet new_bullet = { .type = 6,
                        .flags = BORK_BULLET_HURTS_PLAYER,
                        .damage = 0 };
                    vec3_set_len(new_bullet.dir, ent_to_plr, 0.4);
                    vec3_add(new_bullet.pos, ent_head, new_bullet.dir);
                    ARR_PUSH(d->bullets, new_bullet);
                }
            }
        }
    }
    bork_entity_update(ent, &d->map);
}

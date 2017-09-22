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
#include "state_play.h"
#include "game_states.h"

#define RANDF   ((float)rand() / RAND_MAX)

void create_spark(struct bork_play_data* d, vec3 pos)
{
    struct bork_particle new_part = {
        .pos = { pos[0], pos[1], pos[2] },
        .vel = { 0, 0, 0 },
        .ticks_left = 30,
        .frame_ticks = 6,
        .start_frame = 0, .end_frame = 5
    };
    ARR_PUSH(d->particles, new_part);
}

void create_sparks(struct bork_play_data* d, vec3 pos, int sparks)
{
    int i;
    for(i = 0; i < sparks; ++i) {
        vec3 spark_pos;
        vec3_set(spark_pos,
            pos[0] + (float)rand() / RAND_MAX - 0.5,
            pos[1] + (float)rand() / RAND_MAX - 0.5,
            pos[2] + (float)rand() / RAND_MAX - 0.5);
        create_spark(d, spark_pos);
    }
}

void create_explosion(struct bork_play_data* d, vec3 pos)
{
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { pos[0], pos[1], pos[2] },
        .light = { 1.5, 1.5, 1, 5 },
        .vel = { 0, 0, 0 },
        .ticks_left = 45,
        .frame_ticks = 9,
        .current_frame = 8,
        .start_frame = 8, .end_frame = 12,
    };
    ARR_PUSH(d->particles, new_part);
}

void create_smoke(struct bork_play_data* d, vec3 pos, vec3 dir, int lifetime)
{
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT | BORK_PARTICLE_DECELERATE,
        .pos = { pos[0], pos[1], pos[2] },
        .vel = { dir[0], dir[1], dir[2] },
        .light = { 1.5, 1.5, 1, 5 },
        .vel = { 0, 0, 0 },
        .ticks_left = lifetime,
        .frame_ticks = 0,
        .current_frame = 16 + rand() % 4,
    };
    ARR_PUSH(d->particles, new_part);
}

#define RANDF   ((float)rand() / RAND_MAX)

void robot_explosion(struct bork_play_data* d, vec3 pos)
{
    vec3 pos_ = { pos[0], pos[1], pos[2] };
    create_explosion(d, pos);
    int num_scraps = 3 + rand() % 3;
    int i;
    for(i = 0; i < num_scraps; ++i) {
        bork_entity_t new_id = bork_entity_new(1);
        struct bork_entity* new_item = bork_entity_get(new_id);
        if(!new_item) continue;
        bork_entity_init(new_item, BORK_ITEM_SCRAPMETAL + (rand() % 3));
        vec3_set(new_item->pos,
            pos_[0] + (RANDF - 0.5),
            pos_[1] + (RANDF - 0.5),
            pos_[2] + (RANDF - 0.5));
        vec3_set(new_item->vel,
            (RANDF - 0.5) * 0.25,
            (RANDF - 0.5) * 0.25,
            (RANDF - 0.2) * 0.25);
        new_item->counter[3] = 600;
        new_item->flags |= BORK_ENTFLAG_SMOKING;
        bork_map_add_item(&d->map, new_id);
    }
}

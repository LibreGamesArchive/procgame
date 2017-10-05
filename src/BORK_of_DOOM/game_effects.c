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

void create_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks)
{
    int i;
    for(i = 0; i < sparks; ++i) {
        vec3 off = {
            (float)rand() / RAND_MAX - 0.5,
            (float)rand() / RAND_MAX - 0.5,
            (float)rand() / RAND_MAX - 0.5 };
        struct bork_particle new_part = {
            .pos = { pos[0] + off[0], pos[1] + off[1], pos[2] + off[2] },
            .vel = { off[0] * expand, off[1] * expand, off[2] * expand },
            .ticks_left = 30,
            .frame_ticks = 6,
            .start_frame = 0, .end_frame = 5 };
        ARR_PUSH(d->particles, new_part);
    }
}

void create_elec_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks)
{
    int i;
    for(i = 0; i < sparks; ++i) {
        vec3 off = {
            (float)rand() / RAND_MAX - 0.5,
            (float)rand() / RAND_MAX - 0.5,
            (float)rand() / RAND_MAX - 0.5 };
        struct bork_particle new_part = {
            .pos = { pos[0] + off[0], pos[1] + off[1], pos[2] + off[2] },
            .vel = { off[0] * expand, off[1] * expand, off[2] * expand },
            .ticks_left = 30,
            .frame_ticks = 6,
            .current_frame = 32,
            .start_frame = 32, .end_frame = 36 };
        ARR_PUSH(d->particles, new_part);
    }
}

void create_explosion(struct bork_play_data* d, vec3 pos, float intensity)
{
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { pos[0], pos[1], pos[2] },
        .light = { 1.5, 1.5, 1, 8.0f * intensity},
        .vel = { 0, 0, 0 },
        .lifetime = 60,
        .ticks_left = 60,
        .frame_ticks = 12,
        .current_frame = 8,
        .start_frame = 8, .end_frame = 12,
    };
    ARR_PUSH(d->particles, new_part);
    create_sparks(d, pos, 0.2, 5);
}

void create_elec_explosion(struct bork_play_data* d, vec3 pos)
{
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { pos[0], pos[1], pos[2] },
        .light = { 1.0, 1.0, 1.5, 8 },
        .vel = { 0, 0, 0 },
        .lifetime = 60,
        .ticks_left = 60,
        .frame_ticks = 12,
        .current_frame = 40,
        .start_frame = 40, .end_frame = 44,
    };
    ARR_PUSH(d->particles, new_part);
    create_elec_sparks(d, pos, 0.2, 5);
}

void create_smoke(struct bork_play_data* d, vec3 pos, vec3 dir, int lifetime)
{
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_SPRITE | BORK_PARTICLE_BOUYANT | BORK_PARTICLE_DECELERATE,
        .pos = { pos[0], pos[1], pos[2] },
        .vel = { dir[0], dir[1], dir[2] },
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
    create_explosion(d, pos, 1);
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

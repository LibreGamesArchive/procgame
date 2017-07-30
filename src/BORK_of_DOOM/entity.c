#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "physics.h"

void bork_entity_init(struct bork_entity* ent, vec3 size)
{
    *ent = (struct bork_entity) {
        .size = { size[0], size[1], size[2] },
    };
}

void bork_entity_push(struct bork_entity* ent, vec3 push)
{
    vec3_add(ent->vel, ent->vel, push);
}

void bork_entity_move(struct bork_entity* ent, struct bork_map* map);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map)
{
    if(ent->dead) return;
    switch(ent->type) {
    case BORK_ENTITY_PLAYER:
        bork_entity_move(ent, map);
        break;
    case BORK_ENTITY_ENEMY:
        //printf("%f, %f, %f\n", ent->pos[0], ent->pos[1], ent->pos[2]);
        bork_entity_move(ent, map);
        break;
    case BORK_ENTITY_ITEM: {
        if(!ent->active) return;
        vec3 ent_pos;
        vec3_dup(ent_pos, ent->pos);
        vec3 move;
        bork_entity_move(ent, map);
        vec3_sub(move, ent_pos, ent->pos);
        if(vec3_len(move) < 0.01) {
            ++ent->still_ticks;
            if(ent->still_ticks >= 10) ent->active = 0;
        } else {
            ent->still_ticks = 0;
            ent->active = 1;
        }
        break;
    }
    default: break;
    }
}

void bork_entity_move(struct bork_entity* ent, struct bork_map* map)
{
    ent->ground = 0;
    vec3_add(ent->vel, ent->vel, (vec3){ 0, 0, -0.02 });
    struct bork_collision coll = {};
    float curr_move = 0;
    float max_move = vec3_vmin(ent->size);
    float full_dist = vec3_len(ent->vel);
    vec3 max_move_dir;
    vec3_set_len(max_move_dir, ent->vel, max_move);
    vec3 new_pos = { ent->pos[0], ent->pos[1], ent->pos[2] };
    int ladder = 0;
    int steps = 0;
    int hit = 0;
    while(curr_move < full_dist) {
        vec3 curr_dest;
        if(curr_move + max_move >= full_dist) {
            vec3_set_len(max_move_dir, ent->vel, full_dist - curr_move);
            curr_move = full_dist;
        } else {
            curr_move += max_move;
        }
        vec3_add(new_pos, new_pos, max_move_dir);
        steps = 0;
        while(bork_map_collide(map, &coll, new_pos, ent->size)
                && (steps++ < 4)) {
            if(ent->type == BORK_ENTITY_ENEMY && !coll.tile) {
                printf("%f, %f, %f\n", coll.push[0], coll.push[1], coll.push[2]);
                printf("%f, %f, %f\n", new_pos[0], new_pos[1], new_pos[2]);
            }
            vec3_add(new_pos, new_pos, coll.push);
            float down_angle = vec3_angle_diff(coll.face_norm, PG_DIR_VEC[PG_UP]);
            if(down_angle < 0.1 * M_PI) ent->ground = 1;
            else if(coll.tile && coll.tile->type == BORK_TILE_LADDER) ladder = 1;
            ++hit;
        }
    }
    vec3_sub(ent->vel, new_pos, ent->pos);
    vec3_dup(ent->pos, new_pos);
    int friction = 0;
    if(ladder) {
        if(ent->dir[1] >= 0) ent->vel[2] = 0.1;
        else ent->vel[2] = -0.05;
        friction = 1;
    } else if(ent->ground) {
        ent->vel[2] = 0;
        friction = 1;
    }
    if(friction) vec3_scale(ent->vel, ent->vel, 0.8);
    if(vec3_len(ent->vel) < 0.0005) vec3_set(ent->vel, 0, 0, 0);
}


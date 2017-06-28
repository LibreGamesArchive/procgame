#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "map_area.h"
#include "physics.h"
#include "entity.h"

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

void bork_entity_move(struct bork_entity* ent, struct bork_map* map)
{
    ent->ground = 0;
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
        vec3_add(curr_dest, new_pos, max_move_dir);
        steps = 0;
        while(bork_map_collide(map, &coll, new_pos, curr_dest, ent->size)
                && (steps++ < 4)) {
            vec3 coll_dir;
            vec3_sub(coll_dir, new_pos, curr_dest);
            vec3_dup(curr_dest, new_pos);
            float down_angle = vec3_angle_diff(coll.norm, BORK_DIR_VEC[BORK_UP]);
            if(down_angle < 0.1 * M_PI) ent->ground = 1;
            else if(coll.tile->type == BORK_TILE_LADDER) ladder = 1;
            ++hit;
        }
    }
    vec3_sub(ent->vel, new_pos, ent->pos);
    vec3_dup(ent->pos, new_pos);
    int friction = 0;
    if(ladder) {
        ent->vel[2] = 0.1;
        friction = 1;
    } else if(ent->ground) {
        ent->vel[2] = 0;
        friction = 1;
    }
    if(friction) vec3_scale(ent->vel, ent->vel, 0.8);
    if(vec3_len(ent->vel) < 0.0005) vec3_set(ent->vel, 0, 0, 0);
}


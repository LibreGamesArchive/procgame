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
    vec3 dest_pos;
    vec3_add(dest_pos, ent->pos, ent->vel);
    vec3 new_pos = { dest_pos[0], dest_pos[1], dest_pos[2] };
    int ladder = 0;
    int hit = 0, steps = 0;
    while(bork_map_collide(map, &coll, new_pos, new_pos, ent->size) && steps++ < 5) {
        vec3 coll_dir;
        vec3_sub(coll_dir, new_pos, dest_pos);
        vec3_normalize(coll_dir, coll_dir);
        vec3 down_dir = { 0, 0, 1 };
        float down_angle = vec3_angle_diff(coll_dir, down_dir);
        /*  If there was a collision with an angle nearly directly down,
            mark the entity as being on the ground  */
        if(down_angle < 0.1 * M_PI) {
            ent->ground = 1;
        } else if(coll.tile->type == BORK_TILE_LADDER) {
            ladder = 1;
        }
        ++hit;
    }
    vec3_sub(ent->vel, new_pos, ent->pos);
    vec3_dup(ent->pos, new_pos);
    int friction = 0;
    if(ent->ground) friction = 1;
    if(ladder) {
        friction = 1;
        ent->vel[2] = 0.1;
    }
    if(friction) vec3_scale(ent->vel, ent->vel, 0.8);
    if(vec3_len(ent->vel) < 0.0005) vec3_set(ent->vel, 0, 0, 0);
}


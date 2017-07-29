#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "bullet.h"
#include "physics.h"

void bork_bullet_init(struct bork_bullet* blt, vec3 pos, vec3 dir)
{
    *blt = (struct bork_bullet) {
        .pos = { pos[0], pos[1], pos[2] },
        .dir = { dir[0], dir[1], dir[2] } };
}

void bork_bullet_move(struct bork_bullet* blt, struct bork_map* map)
{
    struct bork_collision coll = {};
    float curr_move = 0;
    float max_move = 0.1;
    float full_dist = vec3_len(blt->dir);
    vec3 max_move_dir;
    vec3_set_len(max_move_dir, blt->dir, max_move);
    vec3 new_pos = { blt->pos[0], blt->pos[1], blt->pos[2] };
    while(curr_move < full_dist) {
        if(curr_move + max_move >= full_dist) {
            vec3_set_len(max_move_dir, blt->dir, full_dist - curr_move);
            curr_move = full_dist;
        } else {
            curr_move += max_move;
        }
        vec3_add(new_pos, new_pos, max_move_dir);
        if(bork_map_collide(map, &coll, new_pos, (vec3){ 0.1, 0.1, 0.1 })) {
            blt->dead_ticks = 10;
            vec3_set(blt->dir, 0, 0, 0);
            vec3_sub(blt->pos, new_pos, max_move_dir);
            vec3_set(blt->light_color, 1, 1, 0.6);
            break;
        }
        /*  Hit the closest entity  */
        enum bork_area area = bork_map_get_area(map,
            blt->pos[0] * 0.5, blt->pos[1] * 0.5, blt->pos[2] * 0.5);
        if(area >= BORK_AREA_EXTERIOR) continue;
        float closest = 100;
        struct bork_entity* closest_ent = NULL;
        struct bork_entity* ent;
        int i;
        ARR_FOREACH_PTR(map->enemies[area], ent, i) {
            vec3 blt_to_ent;
            vec3_sub(blt_to_ent, blt->pos, ent->pos);
            float dist = vec3_len(blt_to_ent);
            if(dist < 0.9 && dist < closest) {
                closest = dist;
                closest_ent = ent;
            }
        }
        if(closest_ent) {
            closest_ent->dead = 1;
            blt->dead_ticks = 30;
            vec3_set(blt->dir, 0, 0, 0);
            vec3_set(blt->light_color, 1, 0.4, 0.4);
        }
    }
    vec3_dup(blt->pos, new_pos);
}

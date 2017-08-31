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
            blt->flags |= BORK_BULLET_DEAD;
            blt->dead_ticks = 10;
            vec3_set(blt->dir, 0, 0, 0);
            vec3_sub(blt->pos, new_pos, max_move_dir);
            vec3_set(blt->light_color, 1, 1, 0.6);
            break;
        }
        if(blt->flags & BORK_BULLET_HURTS_ENEMY) {
            /*  Hit the closest enemy  */
            float closest = 100;
            struct bork_entity* closest_ent = NULL;
            int i;
            bork_entity_t ent_id;
            struct bork_entity* ent;
            ARR_FOREACH(map->enemies, ent_id, i) {
                ent = bork_entity_get(ent_id);
                if(!ent) continue;
                vec3 blt_to_ent;
                vec3_sub(blt_to_ent, blt->pos, ent->pos);
                float dist = vec3_len(blt_to_ent);
                if(dist < 0.9 && dist < closest) {
                    closest = dist;
                    closest_ent = ent;
                }
            }
            if(closest_ent) {
                closest_ent->HP -= 15;
                blt->flags |= BORK_BULLET_DEAD;
                blt->dead_ticks = 10;
                vec3 knockback;
                vec3_set_len(knockback, blt->dir, 0.3);
                vec3_add(closest_ent->vel, closest_ent->vel, knockback);
                vec3_sub(blt->pos, new_pos, blt->dir);
                vec3_set(blt->dir, 0, 0, 0);
                vec3_set(blt->light_color, 1, 1, 0.6);
                break;
            }
        }
        if(blt->flags & BORK_BULLET_HURTS_PLAYER) {
            vec3 blt_to_plr;
            vec3_sub(blt_to_plr, blt->pos, map->plr->pos);
            float dist = vec3_len(blt_to_plr);
            if(dist < 0.8) {
                blt->flags |= BORK_BULLET_DEAD;
                blt->dead_ticks = 10;
                map->plr->HP -= 20;
                vec3_sub(blt->pos, new_pos, blt->dir);
                vec3_set(blt->dir, 0, 0, 0);
                vec3_set(blt->light_color, 2, 0.8, 0.8);
                break;
            }
        }
    }
    vec3_dup(blt->pos, new_pos);
}

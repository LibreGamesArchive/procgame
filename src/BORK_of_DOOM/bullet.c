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
    static bork_entity_arr_t surr = {};
    ARR_TRUNCATE(surr, 0);
    vec3 surr_start, surr_end;
    vec3 surr_center;
    vec3_add(surr_center, blt->pos,
        (vec3){ blt->dir[0] * 0.5, blt->dir[1] * 0.5, blt->dir[2] * 0.5 });
    vec3_sub(surr_start, surr_center, (vec3){ 3, 3, 3 });
    vec3_add(surr_end, surr_center, (vec3){ 3, 3, 3 });
    bork_map_query_enemies(map, &surr, surr_start, surr_end);
    bork_map_query_entities(map, &surr, surr_start, surr_end);
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
        if(blt->flags & BORK_BULLET_HURTS_ENEMY) {
            /*  Hit the closest enemy  */
            float closest = 100;
            struct bork_entity* closest_ent = NULL;
            int i;
            bork_entity_t ent_id;
            struct bork_entity* ent;
            ARR_FOREACH(surr, ent_id, i) {
                ent = bork_entity_get(ent_id);
                if(!ent) continue;
                const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
                vec3 blt_to_ent;
                vec3_sub(blt_to_ent, blt->pos, ent->pos);
                float dist = vec3_len(blt_to_ent);
                if(ent->flags & BORK_ENTFLAG_ENTITY) dist *= 0.5;
                if(dist < prof->size[0] && dist < closest) {
                    closest = dist;
                    closest_ent = ent;
                }
            }
            if(closest_ent) {
                closest_ent->HP -= blt->damage;
                blt->flags |= BORK_BULLET_DEAD;
                blt->dead_ticks = 10;
                if(!(closest_ent->flags & BORK_ENTFLAG_STATIONARY)) {
                    vec3 knockback;
                    vec3_set_len(knockback, blt->dir, 0.1);
                    vec3_add(closest_ent->vel, closest_ent->vel, knockback);
                }
                vec3_set(blt->dir, 0, 0, 0);
                vec3_set(blt->light_color, 1, 1, 0.6);
                if(blt->type == BORK_ITEM_BULLETS_INC - BORK_ITEM_BULLETS
                || blt->type == BORK_ITEM_SHELLS_INC - BORK_ITEM_BULLETS) {
                    closest_ent->flags |= BORK_ENTFLAG_ON_FIRE;
                    closest_ent->fire_ticks = 360;
                }
                return;
            }
        }
        if(blt->flags & BORK_BULLET_HURTS_PLAYER) {
            vec3 blt_to_plr;
            vec3_sub(blt_to_plr, blt->pos, map->plr->pos);
            float dist = vec3_len(blt_to_plr);
            if(dist < 0.8) {
                blt->flags |= BORK_BULLET_DEAD;
                blt->dead_ticks = 10;
                map->plr->pain_ticks = 120;
                map->plr->HP -= blt->damage;
                vec3_sub(blt->pos, new_pos, blt->dir);
                vec3_set(blt->dir, 0, 0, 0);
                vec3_set(blt->light_color, 2, 0.8, 0.8);
                return;
            }
        }
        if(bork_map_check_sphere(map, new_pos, 0.1)) {
            blt->flags |= BORK_BULLET_DEAD;
            blt->dead_ticks = 10;
            vec3_set(blt->dir, 0, 0, 0);
            vec3_sub(blt->pos, new_pos, max_move_dir);
            vec3_set(blt->light_color, 1, 1, 0.6);
            if(blt->type == BORK_ITEM_BULLETS_INC - BORK_ITEM_BULLETS
            || blt->type == BORK_ITEM_SHELLS_INC - BORK_ITEM_BULLETS) {
                bork_map_create_fire(map, blt->pos, 360);
            }
            return;
        }
    }
    vec3_dup(blt->pos, new_pos);
}

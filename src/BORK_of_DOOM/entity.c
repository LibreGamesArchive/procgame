#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "bullet.h"
#include "map_area.h"
#include "physics.h"
#include "game_states.h"

static ARR_T(struct bork_entity) ent_pool = {};

bork_entity_t bork_entity_new(int n)
{
    int run = 1;
    int i = 0;
    int alloc = 0;
    for(i = 0; i < ent_pool.cap; ++i) {
        if(!(ent_pool.data[i].flags & BORK_ENTFLAG_DEAD)) continue;
        run = 1;
        while(run < n && run + i < ent_pool.cap
        && (ent_pool.data[run + i].flags & BORK_ENTFLAG_DEAD)) ++run;
        if(run < n) i += run - 1;
        else break;
    }
    alloc = i + run - 1;
    if(i == ent_pool.cap) ARR_RESERVE(ent_pool, ent_pool.cap + n);
    for(i = 0; i < n; ++i) ent_pool.data[alloc + i].flags = 0;
    return alloc;
}

void bork_entity_init(struct bork_entity* ent, enum bork_entity_type type)
{
    *ent = (struct bork_entity) {
        .type = type,
        .flags = BORK_ENT_PROFILES[type].base_flags,
    };
}
struct bork_entity* bork_entity_get(bork_entity_t ent)
{
    if(ent >= ent_pool.cap || (ent_pool.data[ent].flags & BORK_ENTFLAG_DEAD) || ent < 0) return NULL;
    return &ent_pool.data[ent];
}

void bork_entity_push(struct bork_entity* ent, vec3 push)
{
    vec3_add(ent->vel, ent->vel, push);
}

void bork_entity_move(struct bork_entity* ent, struct bork_map* map);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map)
{
    if((ent->flags & BORK_ENTFLAG_DEAD) || (ent->flags & BORK_ENTFLAG_INACTIVE))
        return;
    vec3 ent_pos, move;
    vec3_dup(ent_pos, ent->pos);
    bork_entity_move(ent, map);
    vec3_sub(move, ent_pos, ent->pos);
    if((ent->flags & BORK_ENTFLAG_ITEM) && vec3_len(move) < 0.01) {
        ++ent->still_ticks;
        if(ent->still_ticks >= 10) ent->flags |= BORK_ENTFLAG_INACTIVE;
    } else {
        ent->still_ticks = 0;
        ent->flags &= ~BORK_ENTFLAG_INACTIVE;
    }
}

void bork_entity_move(struct bork_entity* ent, struct bork_map* map)
{
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    ent->flags &= ~BORK_ENTFLAG_GROUND;
    vec3_add(ent->vel, ent->vel, (vec3){ 0, 0, -0.02 });
    struct bork_collision coll = {};
    float curr_move = 0;
    float max_move = vec3_vmin(prof->size);
    float full_dist = vec3_len(ent->vel);
    vec3 max_move_dir;
    vec3_set_len(max_move_dir, ent->vel, max_move);
    vec3 new_pos = { ent->pos[0], ent->pos[1], ent->pos[2] };
    int ladder = 0;
    int steps = 0;
    int hit = 0;
    while(curr_move < full_dist) {
        if(curr_move + max_move >= full_dist) {
            vec3_set_len(max_move_dir, ent->vel, full_dist - curr_move);
            curr_move = full_dist;
        } else {
            curr_move += max_move;
        }
        vec3_add(new_pos, new_pos, max_move_dir);
        steps = 0;
        while(bork_map_collide(map, &coll, new_pos, prof->size) && (steps++ < 4)) {
            vec3_add(new_pos, new_pos, coll.push);
            float down_angle = vec3_angle_diff(coll.face_norm, PG_DIR_VEC[PG_UP]);
            if(down_angle < 0.1 * M_PI) ent->flags |= BORK_ENTFLAG_GROUND;
            if(coll.tile && coll.tile->type == BORK_TILE_LADDER) ladder = 1;
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
    } else if(ent->flags & BORK_ENTFLAG_GROUND) {
        ent->vel[2] = 0;
        friction = 1;
    }
    if(friction) vec3_scale(ent->vel, ent->vel, 0.8);
    if(vec3_len(ent->vel) < 0.0005) vec3_set(ent->vel, 0, 0, 0);
}

/*  Item use functions  */
void bork_use_machinegun(struct bork_entity* ent, struct bork_play_data* d)
{
    if(ent->counter[0] > d->ticks || d->ammo_bullets <= 0) return;
    ent->counter[0] = d->ticks + 10;
    --d->ammo_bullets;
    vec3 bullet_dir;
    spherical_to_cartesian(bullet_dir, (vec2){ d->plr.dir[0] - M_PI,
                                               d->plr.dir[1] - (M_PI * 0.5) });
    vec3_scale(bullet_dir, bullet_dir, 1);
    struct bork_bullet new_bullet = { .type = 0, .flags = BORK_BULLET_HURTS_ENEMY };
    vec3_dup(new_bullet.pos, d->plr.pos);
    new_bullet.pos[0] += 0.2 * sin(d->plr.dir[0]) + bullet_dir[0] * 0.3;
    new_bullet.pos[1] -= 0.2 * cos(d->plr.dir[0]) - bullet_dir[1] * 0.3;
    new_bullet.pos[2] += 0.65 + bullet_dir[2] * 0.3;
    vec3_dup(new_bullet.dir, bullet_dir);
    ARR_PUSH(d->bullets, new_bullet);
}

void bork_hud_machinegun(struct bork_entity* ent, struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_text;
    struct pg_shader_text text = { .use_blocks = 1 };
    float ar = d->core->aspect_ratio;
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    snprintf(text.block[0], 64, "AMMO: %d", d->ammo_bullets);
    vec4_set(text.block_style[0], ar - 0.6, 0.85, 0.025, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
}

#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "bullet.h"
#include "map_area.h"
#include "physics.h"
#include "particle.h"
#include "upgrades.h"
#include "recycler.h"
#include "state_play.h"
#include "game_states.h"

#define RANDF   ((float)rand() / RAND_MAX)

/*  Item use functions  */
void bork_use_pistol(struct bork_entity* ent, struct bork_play_data* d)
{
    /*  Recoil/game logic   */
    if(ent->counter[0] > d->play_ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->play_ticks + 20;
    --ent->ammo;
    /*  Shoot a bullet  */
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    int ammo_type = prof->use_ammo[ent->ammo_type];
    const struct bork_entity_profile* ammo_prof = &BORK_ENT_PROFILES[ammo_type];
    mat4 view;
    bork_entity_get_view(&d->plr, view);
    vec4 gun_pos = { -0.4, 0.25, -0.2, 1 };
    vec3 bullet_dir = { -1, 0, 0 };
    mat3_mul_vec3(bullet_dir, view, bullet_dir);
    mat4_mul_vec4(gun_pos, view, gun_pos);
    struct bork_bullet new_bullet =
        { .type = ammo_type - BORK_ITEM_BULLETS,
          .flags = BORK_BULLET_HURTS_ENEMY,
          .damage = prof->damage + ammo_prof->damage,
          .range = 120 };
    vec3_dup(new_bullet.pos, gun_pos);
    vec3_dup(new_bullet.dir, bullet_dir);
    ARR_PUSH(d->bullets, new_bullet);
    /*  HUD animation   */
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.2, 0, 0);
    d->hud_angle[1] = -0.3;
    d->hud_anim_speed = 0.04;
    d->hud_anim_active = 2;
    /*  Gunshot effect  */
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { gun_pos[0] + bullet_dir[0], gun_pos[1] + bullet_dir[1],
                 gun_pos[2] + bullet_dir[2] },
        .light = { 1.5, 1.5, 1, 4.0f },
        .vel = { 0, 0, 0 },
        .lifetime = 8,
        .ticks_left = 8,
    };
    ARR_PUSH(d->particles, new_part);
}

void bork_hud_pistol(struct bork_entity* ent, struct bork_play_data* d)
{
    float ar = d->core->aspect_ratio;
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    int ammo_type = prof->use_ammo[ent->ammo_type];
    const struct bork_entity_profile* ammo_prof = &BORK_ENT_PROFILES[ammo_type];
    int ammo_frame = ammo_prof->front_frame;
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.5 }, (vec4){});
    pg_shader_2d_texture(shader, &d->core->item_tex);
    pg_shader_2d_tex_frame(shader, ammo_frame);
    pg_shader_2d_transform(shader, (vec2){ ar - 0.18, 0.66 }, (vec2){ 0.05, 0.05 }, 0);
    pg_model_begin(&d->core->quad_2d_ctr, shader);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    struct pg_shader_text text = { .use_blocks = 2 };
    int ammo_held = d->ammo[ammo_type - BORK_ITEM_BULLETS];
    int ammo_cap = prof->ammo_capacity;
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int len = snprintf(text.block[0], 64, "%d / %d", ent->ammo, ammo_cap);
    vec4_set(text.block_style[0], ar - 0.18 - (len * 0.025 * 1.2 * 0.5), 0.65, 0.025, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    len = snprintf(text.block[1], 64, "%d", ammo_held);
    vec4_set(text.block_style[1], ar - 0.18 - (len * 0.025 * 1.2 * 0.5), 0.69, 0.025, 1.2);
    vec4_set(text.block_color[1], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
}

void bork_use_shotgun(struct bork_entity* ent, struct bork_play_data* d)
{
    /*  Recoil/game logic   */
    if(ent->counter[0] > d->play_ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->play_ticks + 60;
    --ent->ammo;
    /*  Shoot a bullet  */
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    int ammo_type = prof->use_ammo[ent->ammo_type];
    const struct bork_entity_profile* ammo_prof = &BORK_ENT_PROFILES[ammo_type];
    mat4 view;
    bork_entity_get_view(&d->plr, view);
    vec4 gun_pos = { -0.4, 0.25, -0.2, 1 };
    mat4_mul_vec4(gun_pos, view, gun_pos);
    vec3 bullet_dir[5];
    int i;
    for(i = 0; i < 5; ++i) {
        vec3_set(bullet_dir[i], -1,
            (double)rand() / RAND_MAX * 0.2 - 0.1,
            (double)rand() / RAND_MAX * 0.2 - 0.1);
        vec3_set_len(bullet_dir[i], bullet_dir[i], 1);
        mat3_mul_vec3(bullet_dir[i], view, bullet_dir[i]);
        struct bork_bullet new_bullet =
            { .type = ammo_type - BORK_ITEM_BULLETS,
              .flags = BORK_BULLET_HURTS_ENEMY,
              .damage = prof->damage + ammo_prof->damage,
              .range = 120 };
        vec3_dup(new_bullet.pos, gun_pos);
        vec3_dup(new_bullet.dir, bullet_dir[i]);
        ARR_PUSH(d->bullets, new_bullet);
    }
    /*  HUD animation   */
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.3, 0, 0.04);
    vec3_set(d->hud_anim[2], 0.2, 0, 0);
    vec3_set(d->hud_anim[3], 0, 0, 0);
    d->hud_angle[1] = -0.1;
    d->hud_angle[2] = -0.025;
    d->hud_anim_speed = 0.015;
    d->hud_anim_active = 3;
    /*  Gunshot effect  */
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { gun_pos[0] + bullet_dir[0][0], gun_pos[1] + bullet_dir[0][1],
                 gun_pos[2] + bullet_dir[0][2] },
        .light = { 1.5, 1.5, 1, 4.0f },
        .vel = { 0, 0, 0 },
        .lifetime = 4,
        .ticks_left = 4,
    };
    ARR_PUSH(d->particles, new_part);
}

void bork_hud_shotgun(struct bork_entity* ent, struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_text;
    struct pg_shader_text text = { .use_blocks = 1 };
    float ar = d->core->aspect_ratio;
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int len = snprintf(text.block[0], 64, "AMMO: %d", ent->ammo);
    vec4_set(text.block_style[0], ar - 0.2 - (len * 0.025 * 1.2 * 0.5), 0.675, 0.025, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
}

void bork_use_machinegun(struct bork_entity* ent, struct bork_play_data* d)
{
    /*  Recoil/game logic   */
    if(ent->counter[0] > d->play_ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->play_ticks + 20;
    --ent->ammo;
    /*  Shoot a bullet  */
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    int ammo_type = prof->use_ammo[ent->ammo_type];
    const struct bork_entity_profile* ammo_prof = &BORK_ENT_PROFILES[ammo_type];
    mat4 view;
    bork_entity_get_view(&d->plr, view);
    vec4 gun_pos = { -0.4, 0.25, -0.2, 1 };
    vec3 bullet_dir = { -1, 0, 0 };
    mat3_mul_vec3(bullet_dir, view, bullet_dir);
    mat4_mul_vec4(gun_pos, view, gun_pos);
    struct bork_bullet new_bullet =
        { .type = ammo_type - BORK_ITEM_BULLETS,
          .flags = BORK_BULLET_HURTS_ENEMY,
          .damage = prof->damage + ammo_prof->damage,
          .range = 120 };
    vec3_dup(new_bullet.pos, gun_pos);
    vec3_dup(new_bullet.dir, bullet_dir);
    ARR_PUSH(d->bullets, new_bullet);
    /*  HUD animation   */
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.3, 0, 0.02);
    vec3_set(d->hud_anim[2], 0.025, 0, 0);
    vec3_set(d->hud_anim[3], 0, 0, 0);
    d->hud_anim_speed = 0.035;
    d->hud_anim_active = 2;
    /*  Gunshot effect  */
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { gun_pos[0] + bullet_dir[0], gun_pos[1] + bullet_dir[1],
                 gun_pos[2] + bullet_dir[2] },
        .light = { 1.5, 1.5, 1, 4.0f },
        .vel = { 0, 0, 0 },
        .lifetime = 4,
        .ticks_left = 4,
    };
    ARR_PUSH(d->particles, new_part);
}

void bork_hud_machinegun(struct bork_entity* ent, struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_text;
    struct pg_shader_text text = { .use_blocks = 1 };
    float ar = d->core->aspect_ratio;
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int len = snprintf(text.block[0], 64, "AMMO: %d", ent->ammo);
    vec4_set(text.block_style[0], ar - 0.2 - (len * 0.025 * 1.2 * 0.5), 0.675, 0.025, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
}

/*  Item use functions  */
void bork_use_plazgun(struct bork_entity* ent, struct bork_play_data* d)
{
    /*  Recoil/game logic   */
    if(ent->counter[0] > d->play_ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->play_ticks + 40;
    --ent->ammo;
    /*  Shoot a bullet  */
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[ent->type];
    int ammo_type = prof->use_ammo[ent->ammo_type];
    const struct bork_entity_profile* ammo_prof = &BORK_ENT_PROFILES[ammo_type];
    mat4 view;
    bork_entity_get_view(&d->plr, view);
    vec4 gun_pos = { -0.4, 0.25, -0.2, 1 };
    vec3 bullet_dir = { -0.25, 0, 0 };
    mat3_mul_vec3(bullet_dir, view, bullet_dir);
    mat4_mul_vec4(gun_pos, view, gun_pos);
    struct bork_bullet new_bullet =
        { .type = ammo_type - BORK_ITEM_BULLETS,
          .flags = BORK_BULLET_HURTS_ENEMY,
          .damage = prof->damage + ammo_prof->damage,
          .range = 120 };
    vec3_dup(new_bullet.pos, gun_pos);
    vec3_dup(new_bullet.dir, bullet_dir);
    ARR_PUSH(d->bullets, new_bullet);
    /*  HUD animation   */
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.3, 0, 0.04);
    vec3_set(d->hud_anim[2], 0.2, 0, 0);
    vec3_set(d->hud_anim[3], 0, 0, 0);
    d->hud_angle[1] = -0.1;
    d->hud_angle[2] = -0.025;
    d->hud_anim_speed = 0.015;
    d->hud_anim_active = 3;
    /*  Gunshot effect  */
    struct bork_particle new_part = {
        .flags = BORK_PARTICLE_LIGHT | BORK_PARTICLE_LIGHT_DECAY,
        .pos = { gun_pos[0] + bullet_dir[0], gun_pos[1] + bullet_dir[1],
                 gun_pos[2] + bullet_dir[2] },
        .light = { 1.5, 0.5, 0.5, 8.0f },
        .vel = { 0, 0, 0 },
        .lifetime = 30,
        .ticks_left = 30,
    };
    ARR_PUSH(d->particles, new_part);
}

void bork_hud_plazgun(struct bork_entity* ent, struct bork_play_data* d)
{
    struct pg_shader* shader = &d->core->shader_text;
    struct pg_shader_text text = { .use_blocks = 1 };
    float ar = d->core->aspect_ratio;
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int len = snprintf(text.block[0], 64, "AMMO: %d", ent->ammo);
    vec4_set(text.block_style[0], ar - 0.2 - (len * 0.025 * 1.2 * 0.5), 0.675, 0.025, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
}

void bork_use_firstaid(struct bork_entity* ent, struct bork_play_data* d)
{
    if(d->hud_anim_active && d->hud_anim_destroy_when_finished) return;
    bork_play_reset_hud_anim(d);
    d->plr.HP = MIN(100, d->plr.HP + 20);
    vec3_set(d->hud_anim[1], -0.3, -0.3, 0);
    vec3_set(d->hud_anim[2], 0, 0, 0);
    vec3_set(d->hud_anim[3], 0, 0, 0);
    d->hud_anim_speed = 0.005;
    d->hud_anim_active = 2;
    if(--ent->item_quantity == 0) {
        d->hud_anim_destroy_when_finished = 1;
    }
}

static void melee_callback(struct bork_entity* item, struct bork_play_data* d)
{
    const struct bork_entity_profile* item_prof = &BORK_ENT_PROFILES[item->type];
    mat4 view;
    bork_entity_get_view(&d->plr, view);
    vec4 gun_pos = { 0, 0, 0, 1 };
    vec3 bullet_dir = { -0.25, 0, 0 };
    mat3_mul_vec3(bullet_dir, view, bullet_dir);
    mat4_mul_vec4(gun_pos, view, gun_pos);
    struct bork_bullet new_bullet =
        { .type = 31,
          .flags = BORK_BULLET_HURTS_ENEMY,
          .damage = item_prof->damage +
                    item_prof->damage * (get_upgrade_level(d, BORK_UPGRADE_STRENGTH) + 1),
          .range = 2.25 };
    vec3_dup(new_bullet.pos, gun_pos);
    vec3_dup(new_bullet.dir, bullet_dir);
    ARR_PUSH(d->bullets, new_bullet);
}

void bork_use_melee(struct bork_entity* ent, struct bork_play_data* d)
{
    if(d->hud_anim_active) return;
    const struct bork_entity_profile* item_prof = &BORK_ENT_PROFILES[ent->type];
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.3, 0, 0);
    vec3_set(d->hud_anim[2], -0.6, -0.3, 0);
    d->hud_angle[1] = -2.0;
    d->hud_angle[2] = 0.25;
    d->hud_angle[3] = -0.45;
    d->hud_anim_speed = (float)item_prof->speed * 0.005;
    d->hud_anim_active = 3;
    d->hud_anim_callback = melee_callback;
    d->hud_callback_frame = 2;
}

static void grenade_callback(struct bork_entity* item, struct bork_play_data* d)
{
    bork_entity_t ent_id = remove_inventory_item(d, d->held_item);
    struct bork_entity* ent = bork_entity_get(ent_id);
    if(!ent) return;
    mat4 view;
    bork_entity_get_view(&d->plr, view);
    vec4 nade_pos = { 0, 0.2, -0.2, 1 };
    vec3 nade_dir = { -0.4, 0, 0 };
    mat3_mul_vec3(nade_dir, view, nade_dir);
    mat4_mul_vec4(nade_pos, view, nade_pos);
    vec3_dup(ent->pos, nade_pos);
    vec3_dup(ent->vel, nade_dir);
    ent->flags |= BORK_ENTFLAG_BOUNCE | BORK_ENTFLAG_ACTIVE_FUNC | BORK_ENTFLAG_NOT_INTERACTIVE;
    ent->dead_ticks = PLAY_SECONDS(2);
    bork_map_add_item(&d->map, ent_id);
}

void bork_use_grenade(struct bork_entity* ent, struct bork_play_data* d)
{
    if(d->hud_anim_active) return;
    const struct bork_entity_profile* item_prof = &BORK_ENT_PROFILES[ent->type];
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.3, 0, 0);
    vec3_set(d->hud_anim[2], -0.6, 0, 0.25);
    d->hud_angle[1] = -2.0;
    d->hud_angle[2] = 0.25;
    d->hud_angle[3] = -0.45;
    d->hud_anim_speed = 0.015;
    d->hud_anim_active = 3;
    d->hud_anim_callback = grenade_callback;
    d->hud_callback_frame = 2;
}

void bork_tick_grenade(struct bork_entity* ent, struct bork_play_data* d)
{
    static bork_entity_arr_t surr = {};
    if(ent->flags & BORK_ENTFLAG_DEAD) return;
    --ent->dead_ticks;
    if(ent->dead_ticks <= 0) {
        game_explosion(d, ent->pos, 1);
        ent->flags |= BORK_ENTFLAG_DEAD;
    }
}

void bork_tick_grenade_emp(struct bork_entity* ent, struct bork_play_data* d)
{
    static bork_entity_arr_t surr = {};
    if(ent->flags & BORK_ENTFLAG_DEAD) return;
    --ent->dead_ticks;
    if(ent->dead_ticks <= 0) {
        ARR_TRUNCATE(surr, 0);
        vec3 start, end;
        vec3_sub(start, ent->pos, (vec3){ 5, 5, 5 });
        vec3_add(end, ent->pos, (vec3){ 5, 5, 5 });
        bork_map_query_enemies(&d->map, &surr, start, end);
        int i;
        bork_entity_t surr_ent_id;
        struct bork_entity* surr_ent;
        ARR_FOREACH(surr, surr_ent_id, i) {
            surr_ent = bork_entity_get(surr_ent_id);
            if(!surr_ent) continue;
            float dist = vec3_dist(ent->pos, surr_ent->pos);
            if(dist > 5.0f) continue;
            surr_ent->flags |= BORK_ENTFLAG_EMP;
            surr_ent->emp_ticks = PLAY_SECONDS(5);
        }
        create_elec_explosion(d, ent->pos);
        ent->flags |= BORK_ENTFLAG_DEAD;
    }
}

void bork_tick_grenade_inc(struct bork_entity* ent, struct bork_play_data* d)
{
    static bork_entity_arr_t surr = {};
    if(ent->flags & BORK_ENTFLAG_DEAD) return;
    --ent->dead_ticks;
    if(ent->dead_ticks <= 0) {
        ARR_TRUNCATE(surr, 0);
        vec3 start, end;
        vec3_sub(start, ent->pos, (vec3){ 5, 5, 5 });
        vec3_add(end, ent->pos, (vec3){ 5, 5, 5 });
        bork_map_query_enemies(&d->map, &surr, start, end);
        int i;
        bork_entity_t surr_ent_id;
        struct bork_entity* surr_ent;
        ARR_FOREACH(surr, surr_ent_id, i) {
            surr_ent = bork_entity_get(surr_ent_id);
            if(!surr_ent) continue;
            float dist = vec3_dist(ent->pos, surr_ent->pos);
            if(dist > 5.0f) continue;
            surr_ent->flags |= BORK_ENTFLAG_ON_FIRE;
            surr_ent->fire_ticks = PLAY_SECONDS(3);
        }
        create_explosion(d, ent->pos, 1);
        for(i = 0; i < 6; ++i) {
            vec3 off = { ((float)rand() / RAND_MAX - 0.5) * 0.5,
                         ((float)rand() / RAND_MAX - 0.5) * 0.5,
                         ((float)rand() / RAND_MAX) * 0.5 };
            struct bork_fire new_fire = {
                .flags = BORK_FIRE_MOVES,
                .pos = { ent->pos[0] + off[0], ent->pos[1] + off[1], ent->pos[2] + off[2] },
                .vel = { off[0] * 0.25, off[1] * 0.25, off[2] * 0.25 },
                .lifetime = PLAY_SECONDS(10) + PLAY_SECONDS(RANDF * 2) };
            ARR_PUSH(d->map.fires, new_fire);
        }
        ent->flags |= BORK_ENTFLAG_DEAD;
    }
}

void bork_barrel_explode(struct bork_entity* ent, struct bork_play_data* d)
{
    if(ent->counter[0] == 1) {
        game_explosion(d, ent->pos, 1);
        int i;
        for(i = 0; i < 6; ++i) {
            int f = rand() % 3;
            vec3 off = {
                (float)rand() / RAND_MAX - 0.5,
                (float)rand() / RAND_MAX - 0.5,
                (float)rand() / RAND_MAX };
            struct bork_particle new_part = {
                .flags = BORK_PARTICLE_GRAVITY,
                .pos = { ent->pos[0] + off[0], ent->pos[1] + off[1], ent->pos[2] + off[2] },
                .vel = { off[0] * 0.15, off[1] * 0.15, off[2] * 0.15, },
                .ticks_left = 50,
                .frame_ticks = 6,
                .current_frame = 5,
                .start_frame = 5 + f, .end_frame = 5 + f };
            ARR_PUSH(d->particles, new_part);
        }
        ent->flags |= BORK_ENTFLAG_DEAD;
    } else if(ent->counter[0] == 0) {
        ent->counter[0] = PLAY_SECONDS(0.5);
    } else {
        --ent->counter[0];
    }
}

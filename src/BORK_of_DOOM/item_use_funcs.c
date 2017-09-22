#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "bullet.h"
#include "map_area.h"
#include "physics.h"
#include "upgrades.h"
#include "state_play.h"
#include "game_states.h"

/*  Item use functions  */
void bork_use_pistol(struct bork_entity* ent, struct bork_play_data* d)
{
    /*  Recoil/game logic   */
    if(ent->counter[0] > d->ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->ticks + 20;
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
          .damage = prof->damage + ammo_prof->damage };
    vec3_dup(new_bullet.pos, gun_pos);
    vec3_dup(new_bullet.dir, bullet_dir);
    ARR_PUSH(d->bullets, new_bullet);
    /*  HUD animation   */
    bork_play_reset_hud_anim(d);
    vec3_set(d->hud_anim[1], 0.2, 0, 0);
    d->hud_angle[1] = -0.3;
    d->hud_anim_speed = 0.04;
    d->hud_anim_active = 2;
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
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 0.5 });
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
    if(ent->counter[0] > d->ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->ticks + 60;
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
              .damage = prof->damage + ammo_prof->damage };
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
    if(ent->counter[0] > d->ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->ticks + 20;
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
          .damage = prof->damage + ammo_prof->damage };
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
    if(ent->counter[0] > d->ticks || ent->ammo <= 0) return;
    ent->counter[0] = d->ticks + 40;
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
          .damage = prof->damage + ammo_prof->damage };
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
    if(d->looked_enemy == -1) return;
    struct bork_entity* ent = bork_entity_get(d->looked_enemy);
    if(!ent) return;
    vec3 ent_to_plr;
    vec3_sub(ent_to_plr, ent->pos, d->plr.pos);
    if(vec3_len(ent_to_plr) >= 2.5) return;
    vec3 look_dir, look_pos;
    bork_entity_get_eye(&d->plr, look_dir, look_pos);
    if(!bork_map_check_vis(&d->map, look_pos, ent->pos)) return;
    vec3_scale(look_dir, look_dir, 0.3);
    vec3_add(ent->vel, ent->vel, look_dir);
    const struct bork_entity_profile* item_prof = &BORK_ENT_PROFILES[item->type];
    ent->HP -= item_prof->damage;
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

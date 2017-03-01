#include "../arr.h"

struct coll_ring {
    float power;
    float angle;
    vec2 pos;
};

struct collider_state {
    struct pg_shader shader_3d;
    struct pg_shader shader_text;
    struct pg_texture font;
    struct pg_texture ring_texture;
    struct pg_model ring_model;
    struct pg_texture env_texture;
    struct pg_model env_model;
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    float player_angle;
    float player_speed;
    float player_light_intensity;
    vec2 player_pos;
    ARR_T(struct coll_ring) rings;
};

void collider_init(struct collider_state* coll);
void collider_deinit(struct collider_state* coll);
void collider_update(struct collider_state* coll);
void collider_draw(struct collider_state* coll);

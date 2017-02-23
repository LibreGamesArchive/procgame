#include "../arr.h"
#include "ring.h"

struct collider_state {
    struct pg_shader shader_3d;
    struct pg_texture ring_texture;
    struct pg_model ring_model;
    struct pg_viewer view;
    float player_angle;
    vec2 player_pos;
    ARR_T(struct coll_ring) rings;
};

void collider_init(struct collider_state* coll);
void collider_deinit(struct collider_state* coll);
void collider_update(struct collider_state* coll);
void collider_draw(struct collider_state* coll);

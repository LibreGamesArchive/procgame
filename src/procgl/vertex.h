#include <stdint.h>
#include "../linmath.h"
#include "../arr.h"

struct pg_vert3d {
    vec3 pos, normal, tangent, bitangent;
    vec2 tex_coord;
};

struct pg_vert2d {
    vec2 pos;
    vec2 tex_coord;
    uint8_t color[4];
    float tex_weight;
};

typedef ARR_T(struct pg_vert3d) vert3d_buf_t;
typedef ARR_T(struct pg_vert2d) vert2d_buf_t;
typedef ARR_T(unsigned) tri_buf_t;

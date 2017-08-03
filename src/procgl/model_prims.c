#include <GL/glew.h>
#include "ext/linmath.h"
#include "arr.h"
#include "procgl_base.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"
#include "viewer.h"
#include "model.h"
#include "shader.h"

void pg_model_quad(struct pg_model* model, vec2 tex_scale)
{
    pg_model_reset(model);
    uint32_t c = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    model->components = c;
    pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
        .pos = { -0.5, -0.5, 0 }, .uv = { 0, tex_scale[1] } });
    pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
        .pos = { -0.5, 0.5, 0 }, .uv = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
        .pos = { 0.5, -0.5, 0 }, .uv = { tex_scale[0], tex_scale[1] } });
    pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
        .pos = { 0.5, 0.5, 0 }, .uv = { tex_scale[0], 0 } });
    pg_model_add_triangle(model, 0, 2, 1);
    pg_model_add_triangle(model, 1, 2, 3);
}

void pg_model_cube(struct pg_model* model, vec2 tex_scale)
{
    pg_model_reset(model);
    uint32_t c = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    model->components = c;
    vec3 face = { 0, 0.5, 0 };
    vec3 point[4] = { { -0.5, 0, -0.5 },
                      { -0.5, 0, 0.5 },
                      { 0.5, 0, -0.5 },
                      { 0.5, 0, 0.5 } };
    int i, j;
    int idx[4];
    for(i = 0; i < 4; ++i) {
        for(j = 0; j < 4; ++j) {
            idx[j] = pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
                .pos = { point[j][0] + face[0],
                         point[j][1] + face[1],
                         point[j][2] + face[2] },
                .uv = { (j > 1) * tex_scale[0], (j & 1) * tex_scale[1] } });
            /*  Rotate the vertex a quarter-turn for the next face  */
            vec3_set(point[j], -point[j][1], point[j][0], point[j][2]);
        }
        pg_model_add_triangle(model, idx[0], idx[1], idx[2]);
        pg_model_add_triangle(model, idx[1], idx[3], idx[2]);
        /*  Rotate the face vector a quarter-turn so the next face has the
            correct orientation */
        vec3_set(face, -face[1], face[0], face[2]);
    }
    vec3_set(face, face[0], -face[2], face[1]);
    for(j = 0; j < 4; ++j) {
        /*  Rotate the vertex a quarter-turn for the top face   */
        vec3_set(point[j], point[j][0], -point[j][2], point[j][1]);
        idx[j] = pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { point[j][0] + face[0],
                     point[j][1] + face[1],
                     point[j][2] + face[2] },
            .uv = { (j > 1) * tex_scale[0], (j & 1) * tex_scale[1] } });
    }
    pg_model_add_triangle(model, idx[0], idx[1], idx[2]);
    pg_model_add_triangle(model, idx[1], idx[3], idx[2]);
    vec3_set(face, face[0], -face[1], -face[2]);
    for(j = 0; j < 4; ++j) {
        /* Rotate the vertex a half-turn for the bottom face    */
        vec3_set(point[j], point[j][0], -point[j][1], -point[j][2]);
        idx[j] = pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { point[j][0] + face[0],
                     point[j][1] + face[1],
                     point[j][2] + face[2] },
            .uv = { (j > 1) * tex_scale[0], (j & 1) * tex_scale[1] } });
    }
    pg_model_add_triangle(model, idx[0], idx[1], idx[2]);
    pg_model_add_triangle(model, idx[1], idx[3], idx[2]);
}

void pg_model_rect_prism(struct pg_model* model, vec3 scale, vec4* face_uv)
{
    pg_model_reset(model);
    model->components = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    struct pg_vertex_full new_vert = { .components = model->components };
    int i;
    for(i = 0; i < 6; ++i) {
        vec3_add(new_vert.pos, PG_DIR_VEC[i], PG_DIR_TAN[i]);
        vec3_add(new_vert.pos, new_vert.pos, PG_DIR_BITAN[i]);
        vec3_mul(new_vert.pos, new_vert.pos, scale);
        vec2_set(new_vert.uv, face_uv[i][0], face_uv[i][1]);
        unsigned idx = pg_model_add_vertex(model, &new_vert);
        vec3_add(new_vert.pos, PG_DIR_VEC[i], PG_DIR_TAN[i]);
        vec3_sub(new_vert.pos, new_vert.pos, PG_DIR_BITAN[i]);
        vec3_mul(new_vert.pos, new_vert.pos, scale);
        vec2_set(new_vert.uv, face_uv[i][0], face_uv[i][3]);
        pg_model_add_vertex(model, &new_vert);
        vec3_sub(new_vert.pos, PG_DIR_VEC[i], PG_DIR_TAN[i]);
        vec3_add(new_vert.pos, new_vert.pos, PG_DIR_BITAN[i]);
        vec3_mul(new_vert.pos, new_vert.pos, scale);
        vec2_set(new_vert.uv, face_uv[i][2], face_uv[i][1]);
        pg_model_add_vertex(model, &new_vert);
        vec3_sub(new_vert.pos, PG_DIR_VEC[i], PG_DIR_TAN[i]);
        vec3_sub(new_vert.pos, new_vert.pos, PG_DIR_BITAN[i]);
        vec3_mul(new_vert.pos, new_vert.pos, scale);
        vec2_set(new_vert.uv, face_uv[i][2], face_uv[i][3]);
        pg_model_add_vertex(model, &new_vert);
        if(i % 2) {
            pg_model_add_triangle(model, idx, idx + 1, idx + 2);
            pg_model_add_triangle(model, idx + 2, idx + 1, idx + 3);
        } else {
            pg_model_add_triangle(model, idx + 1, idx, idx + 2);
            pg_model_add_triangle(model, idx + 1, idx + 2, idx + 3);
        }
    }
}


void pg_model_cylinder(struct pg_model* model, int n, vec2 tex_scale)
{
    pg_model_reset(model);
    uint32_t c = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    model->components = c;
    float half = (M_PI * 2) / n * 0.5;
    int i;
    for(i = 0; i < n; ++i) {
        float angle = (M_PI * 2) / n * i - half;
        float angle_next = (M_PI * 2) / n * (i + 1) - half;
        float angle_f = (angle + half) / (M_PI * 2);
        float angle_next_f = (angle_next + half) / (M_PI * 2);
        if(i == 0) {
            pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
                .pos = { 0.5 * cos(angle), 0.5 * sin(angle), 0 },
                .uv = { angle_f * tex_scale[0], 0 } });
            pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
                .pos = { 0.5 * cos(angle), 0.5 * sin(angle), 1 },
                .uv = { angle_f * tex_scale[0], tex_scale[1] } });
        }
        pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { 0.5 * cos(angle_next), 0.5 * sin(angle_next), 0 },
            .uv = { angle_next_f * tex_scale[0], 0 } });
        pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { 0.5 * cos(angle_next), 0.5 * sin(angle_next), 1 },
            .uv = { angle_next_f * tex_scale[0], tex_scale[1] } });
        if(i == 0) {
            pg_model_add_triangle(model, 0, 2, 1);
            pg_model_add_triangle(model, 1, 2, 3);
        } else {
            pg_model_add_triangle(model, i * 2, i * 2 + 2, i * 2 + 1);
            pg_model_add_triangle(model, i * 2 + 1, i * 2 + 2, i * 2 + 3);
        }
    }
}

void pg_model_cone(struct pg_model* model, int n, float base,
                   vec3 warp, vec2 tex_scale)
{
    pg_model_reset(model);
    uint32_t c = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    model->components = c;
    pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
        .pos = { warp[0], warp[1], warp[2] },
        .uv = { 0.5, 0.5 } });
    float half = (M_PI * 2) / n * 0.5;
    int i;
    for(i = 0; i < n; ++i) {
        float angle = (M_PI * 2) / n * i - half;
        float angle_next = (M_PI * 2) / n * (i + 1) - half;
        if(i == 0) {
            pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
                .pos = { 0.5 * cos(angle), 0.5 * sin(angle), base },
                .uv = { 0.5 * tex_scale[0] * cos(angle) + 0.5,
                               0.5 * tex_scale[1] * sin(angle) + 0.5 } });
        }
        if(i < n - 1) {
            pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
                .pos = { 0.5 * cos(angle_next), 0.5 * sin(angle_next), base },
                .uv = { 0.5 * tex_scale[0] * cos(angle_next) + 0.5,
                               0.5 * tex_scale[1] * sin(angle_next) + 0.5 } });
        }
        if(i < n - 1) {
            pg_model_add_triangle(model, 0, i + 1, i + 2 );
        } else {
            pg_model_add_triangle(model, 0, n, 1);
        }
    }
}

void pg_model_cone_trunc(struct pg_model* model, int n, float t, vec3 warp,
                         vec2 tex_scale, int tex_repeat)
{
    pg_model_reset(model);
    uint32_t c = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    model->components = c;
    float a = (M_PI * 2) / n;
    float half = a * 0.5;
    float outer_chord = 2 * sin(a / 2);
    float inner_chord = t * outer_chord;
    float top_tex_len = inner_chord / outer_chord;
    float top_tex_start = (1 - top_tex_len) / 2 * tex_scale[0];
    int tex_repeat_tris = 1;
    float tex_step, tex_step_top;
    if(tex_repeat) {
        tex_repeat_tris = n / tex_repeat;
        tex_step = 1 / (float)tex_repeat_tris * tex_scale[0];
        tex_step_top = tex_step * top_tex_len;
    }
    int i;
    vec2 t0, t1, t2, t3;
    for(i = 0; i < n; ++i) {
        float angle = (M_PI * 2) / n * i - half;
        float angle_next = (M_PI * 2) / n * (i + 1) - half;
        if(!tex_repeat) {
            vec2_set(t0, 0.5 + cos(angle) * tex_scale[0] * 0.5,
                         0.5 + sin(angle) * tex_scale[1] * 0.5);
            vec2_set(t1, 0.5 + cos(angle) * t * tex_scale[0] * 0.5,
                         0.5 + sin(angle) * t * tex_scale[1] * 0.5);
            vec2_set(t2, 0.5 + cos(angle_next) * tex_scale[0] * 0.5,
                         0.5 + sin(angle_next) * tex_scale[1] * 0.5);
            vec2_set(t3, 0.5 + cos(angle_next) * t * tex_scale[0] * 0.5,
                         0.5 + sin(angle_next) * t * tex_scale[1] * 0.5);
        } else {
            vec2_set(t0, tex_step * (i % tex_repeat_tris), 0);
            vec2_set(t1, top_tex_start + tex_step_top * (i % tex_repeat_tris), tex_scale[1]);
            vec2_set(t2, tex_step + tex_step * (i % tex_repeat_tris), 0);
            vec2_set(t3, top_tex_start + tex_step_top + tex_step_top * (i % tex_repeat_tris), tex_scale[1]);
        }
        pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { cos(angle) + warp[0], sin(angle) + warp[1], 0 },
            .uv = { t0[0], t0[1] } });
        pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { t * cos(angle) + warp[0], t * sin(angle) + warp[1], warp[2] },
            .uv = { t1[0], t1[1] } });
        pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { cos(angle_next) + warp[0], sin(angle_next) + warp[1], 0 },
            .uv = { t2[0], t2[1] } });
        pg_model_add_vertex(model, &(struct pg_vertex_full) { c,
            .pos = { t * cos(angle_next) + warp[0], t * sin(angle_next) + warp[1], warp[2] },
            .uv = { t3[0], t3[1] } });
        pg_model_add_triangle(model, i * 4, i * 4 + 2, i * 4 + 1);
        pg_model_add_triangle(model, i * 4 + 1, i * 4 + 2, i * 4 + 3);
    }
}

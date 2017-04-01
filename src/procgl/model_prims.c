#include <GL/glew.h>
#include "ext/linmath.h"
#include "vertex.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"
#include "viewer.h"
#include "shader.h"
#include "model.h"

void pg_model_quad(struct pg_model* model, vec2 tex_scale)
{
    pg_model_reset(model);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { -0.5, 0, -0.5 }, .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { -0.5, 0, 0.5 }, .tex_coord = { 0, tex_scale[1] } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { 0.5, 0, -0.5 }, .tex_coord = { tex_scale[0], 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { 0.5, 0, 0.5 }, .tex_coord = { tex_scale[0], tex_scale[1] } });
    pg_model_add_triangle(model, 1, 0, 2);
    pg_model_add_triangle(model, 1, 2, 3);
    pg_model_precalc_verts(model);
}

void pg_model_cube(struct pg_model* model, vec2 tex_scale)
{
    pg_model_reset(model);
    vec3 face = { 0, 0.5, 0 };
    vec3 point[4] = { { -0.5, 0, -0.5 },
                      { -0.5, 0, 0.5 },
                      { 0.5, 0, -0.5 },
                      { 0.5, 0, 0.5 } };
    int i, j;
    int idx[4];
    for(i = 0; i < 4; ++i) {
        for(j = 0; j < 4; ++j) {
            idx[j] = pg_model_add_vertex(model, &(struct pg_vert3d) {
                .pos = { point[j][0] + face[0],
                         point[j][1] + face[1],
                         point[j][2] + face[2] },
                .tex_coord = { (j > 1) * tex_scale[0], (j & 1) * tex_scale[1] } });
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
        idx[j] = pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { point[j][0] + face[0],
                     point[j][1] + face[1],
                     point[j][2] + face[2] },
            .tex_coord = { (j > 1) * tex_scale[0], (j & 1) * tex_scale[1] } });
    }
    pg_model_add_triangle(model, idx[0], idx[1], idx[2]);
    pg_model_add_triangle(model, idx[1], idx[3], idx[2]);
    vec3_set(face, face[0], -face[1], -face[2]);
    for(j = 0; j < 4; ++j) {
        /* Rotate the vertex a half-turn for the bottom face    */
        vec3_set(point[j], point[j][0], -point[j][1], -point[j][2]);
        idx[j] = pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { point[j][0] + face[0],
                     point[j][1] + face[1],
                     point[j][2] + face[2] },
            .tex_coord = { (j > 1) * tex_scale[0], (j & 1) * tex_scale[1] } });
    }
    pg_model_add_triangle(model, idx[0], idx[1], idx[2]);
    pg_model_add_triangle(model, idx[1], idx[3], idx[2]);    
    pg_model_precalc_verts(model);
}

void pg_model_cylinder(struct pg_model* model, int n, vec2 tex_scale)
{
    pg_model_reset(model);
    float half = (M_PI * 2) / n * 0.5;
    int i;
    for(i = 0; i < n; ++i) {
        float angle = (M_PI * 2) / n * i - half;
        float angle_next = (M_PI * 2) / n * (i + 1) - half;
        float angle_f = (angle + half) / (M_PI * 2);
        float angle_next_f = (angle_next + half) / (M_PI * 2);
        if(i == 0) {
            pg_model_add_vertex(model, &(struct pg_vert3d) {
                .pos = { cos(angle), sin(angle), 0 },
                .tex_coord = { angle_f * tex_scale[0], 0 },
                .normal = { cos(angle), sin(angle), 0 } });
            pg_model_add_vertex(model, &(struct pg_vert3d) {
                .pos = { cos(angle), sin(angle), 1 },
                .tex_coord = { angle_f * tex_scale[0], tex_scale[1] },
                .normal = { cos(angle), sin(angle), 0 } });
        }
        pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { cos(angle_next), sin(angle_next), 0 },
            .tex_coord = { angle_next_f * tex_scale[0], 0 },
            .normal = { cos(angle_next), sin(angle_next), 0 } });
        pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { cos(angle_next), sin(angle_next), 1 },
            .tex_coord = { angle_next_f * tex_scale[0], tex_scale[1] },
            .normal = { cos(angle_next), sin(angle_next), 0 } });
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
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { warp[0], warp[1], warp[2] },
        .tex_coord = { 0.5, 0.5 } });
    float half = (M_PI * 2) / n * 0.5;
    int i;
    for(i = 0; i < n; ++i) {
        float angle = (M_PI * 2) / n * i - half;
        float angle_next = (M_PI * 2) / n * (i + 1) - half;
        if(i == 0) {
            pg_model_add_vertex(model, &(struct pg_vert3d) {
                .pos = { 0.5 * cos(angle), 0.5 * sin(angle), base },
                .tex_coord = { 0.5 * tex_scale[0] * cos(angle) + 0.5,
                               0.5 * tex_scale[1] * sin(angle) + 0.5 } });
        }
        if(i < n - 1) {
            pg_model_add_vertex(model, &(struct pg_vert3d) {
                .pos = { 0.5 * cos(angle_next), 0.5 * sin(angle_next), base },
                .tex_coord = { 0.5 * tex_scale[0] * cos(angle_next) + 0.5,
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
        pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { cos(angle) + warp[0], sin(angle) + warp[1], 0 },
            .tex_coord = { t0[0], t0[1] } });
        pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { t * cos(angle) + warp[0], t * sin(angle) + warp[1], warp[2] },
            .tex_coord = { t1[0], t1[1] } });
        pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { cos(angle_next) + warp[0], sin(angle_next) + warp[1], 0 },
            .tex_coord = { t2[0], t2[1] } });
        pg_model_add_vertex(model, &(struct pg_vert3d) {
            .pos = { t * cos(angle_next) + warp[0], t * sin(angle_next) + warp[1], warp[2] },
            .tex_coord = { t3[0], t3[1] } });
        pg_model_add_triangle(model, i * 4, i * 4 + 2, i * 4 + 1);
        pg_model_add_triangle(model, i * 4 + 1, i * 4 + 2, i * 4 + 3);
    }
}

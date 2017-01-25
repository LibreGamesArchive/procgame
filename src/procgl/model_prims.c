#include "renderer.h"
#include "texture.h"
#include "model.h"

void procgl_model_quad(struct procgl_model* model)
{
    ARR_TRUNCATE(model->verts, 0);
    ARR_TRUNCATE(model->tris, 0);
    procgl_model_add_vertex(model, &(struct vertex) {
        .pos = { -0.5, 0, 0.5 }, .tex_coord = { 0, 0 }
    });
    procgl_model_add_vertex(model, &(struct vertex) {
        .pos = { 0.5, 0, 0.5 }, .tex_coord = { 0, 0 }
    });
    procgl_model_add_vertex(model, &(struct vertex) {
        .pos = { -0.5, 0, -0.5 }, .tex_coord = { 0, 0 }
    });
    procgl_model_add_vertex(model, &(struct vertex) {
        .pos = { 0.5, 0, -0.5 }, .tex_coord = { 0, 0 }
    });
    procgl_model_add_triangle(model, 0, 1, 2);
    procgl_model_add_triangle(model, 3, 1, 2);
}

void procgl_model_cube(struct procgl_model* model)
{
    static const vec3 verts[8] = {
        { 0.5, 0.5, 0.5 },
        { -0.5, 0.5, 0.5 },
        { 0.5, -0.5, 0.5 },
        { -0.5, -0.5, 0.5 },
        { 0.5, 0.5, -0.5 },
        { -0.5, 0.5, -0.5 },
        { 0.5, -0.5, -0.5 },
        { -0.5, -0.5, -0.5 } };
    static const unsigned tris[36] = {
        0, 1, 2,
        2, 1, 3,
        5, 4, 6,
        5, 6, 7,
        1, 0, 4,
        1, 4, 5,
        2, 3, 6,
        6, 3, 7,
        0, 2, 4,
        4, 2, 6,
        3, 1, 5,
        3, 5, 7 };
    procgl_model_reserve_verts(model, 8);
    procgl_model_reserve_tris(model, 12);
    int i;
    for(i = 0; i < 8; ++i) {
        model->verts.data[i] = (struct vertex){
            .pos = { verts[i][0], verts[i][1], verts[i][2] } };
    }
    memcpy(model->tris.data, tris, sizeof(tris));
}
#if 0
    vec3 norm = { 0, 1.0, 0 };
    vec3 face = { 0, 0.5, 0 };
    vec3 point[4] = { { -0.5, 0, -0.5 },
                      { -0.5, 0, 0.5 },
                      { 0.5, 0, -0.5 },
                      { 0.5, 0, 0.5 } };
    int i, j;
    int idx[4];
    for(i = 0; i < 4; ++i) {
        for(j = 0; j < 4; ++j) {
            idx[j] = procgl_model_add_vertex(model, &(struct vertex) {
                .pos = { point[j][0] + face[0], point[j][1] + face[1], point[j][2] + face[2] },
                .tex_coord = { (j > 1), (j & 1) } });
            vec3 tmp = { -point[j][1], point[j][0], point[j][2] };
            vec3_set(point[j], tmp[0], tmp[1], tmp[2]);
        }
        procgl_model_add_triangle(model, idx[0], idx[1], idx[2]);
        procgl_model_add_triangle(model, idx[1], idx[3], idx[2]);
        vec3 tmp = { -face[1], face[0], face[2] };
        vec3_dup(face, tmp);
        vec3_set(tmp, -norm[1], norm[0], norm[2]);
        vec3_dup(norm, tmp);
    }
    vec3 tmp = { face[0], -face[2], face[1] };
    vec3_set(face, tmp[0], tmp[1], tmp[2]);
    vec3_set(tmp, norm[0], -norm[2], norm[1]);
    vec3_dup(norm, tmp);
    for(j = 0; j < 4; ++j) {
        vec3 tmp = { point[j][0], -point[j][2], point[j][1] };
        vec3_set(point[j], tmp[0], tmp[1], tmp[2]);
        idx[j] = procgl_model_add_vertex(model, &(struct vertex) {
            .pos = { point[j][0] + face[0], point[j][1] + face[1], point[j][2] + face[2] },
            .tex_coord = { (j > 1), (j & 1) } });
    }
    procgl_model_add_triangle(model, idx[0], idx[1], idx[2]);
    procgl_model_add_triangle(model, idx[1], idx[3], idx[2]);
    vec3_set(tmp, face[0], -face[1], -face[2]);
    vec3_dup(face, tmp);
    vec3_set(tmp, norm[0], -norm[1], -norm[2]);
    vec3_dup(norm, tmp);
    for(j = 0; j < 4; ++j) {
        vec3 tmp = { point[j][0], -point[j][1], -point[j][2] };
        vec3_set(point[j], tmp[0], tmp[1], tmp[2]);
        idx[j] = procgl_model_add_vertex(model, &(struct vertex) {
            .pos = { point[j][0] + face[0], point[j][1] + face[1], point[j][2] + face[2] },
            .tex_coord = { (j > 1), (j & 1) } });
    }
    procgl_model_add_triangle(model, idx[0], idx[1], idx[2]);
    procgl_model_add_triangle(model, idx[1], idx[3], idx[2]);
}
#endif


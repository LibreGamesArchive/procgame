#include <GL/glew.h>
#include "vertex.h"
#include "shader.h"
#include "texture.h"
#include "model.h"

void pg_model_quad(struct pg_model* model)
{
    pg_model_init(model);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { -0.5, 0, 0.5 }, .tex_coord = { 0, 0 }
    });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { 0.5, 0, 0.5 }, .tex_coord = { 0, 0 }
    });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { -0.5, 0, -0.5 }, .tex_coord = { 0, 0 }
    });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { 0.5, 0, -0.5 }, .tex_coord = { 0, 0 }
    });
    pg_model_add_triangle(model, 0, 1, 2);
    pg_model_add_triangle(model, 3, 1, 2);
}

void pg_model_cube(struct pg_model* model)
{
    pg_model_init(model);
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
    pg_model_reserve_verts(model, 8);
    pg_model_reserve_tris(model, 12);
    int i;
    for(i = 0; i < 8; ++i) {
        model->verts.data[i] = (struct pg_vert3d){
            .pos = { verts[i][0], verts[i][1], verts[i][2] } };
    }
    memcpy(model->tris.data, tris, sizeof(tris));
}

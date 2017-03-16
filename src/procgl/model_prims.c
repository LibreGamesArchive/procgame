#include <GL/glew.h>
#include "ext/linmath.h"
#include "vertex.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"
#include "viewer.h"
#include "shader.h"
#include "model.h"

void pg_model_quad(struct pg_model* model,
                   float tex_scale_x, float tex_scale_y)
{
    pg_model_init(model);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { -0.5, 0, -0.5 }, .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { -0.5, 0, 0.5 }, .tex_coord = { 0, tex_scale_y } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { 0.5, 0, -0.5 }, .tex_coord = { tex_scale_x, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { 0.5, 0, 0.5 }, .tex_coord = { tex_scale_x, tex_scale_y } });
    pg_model_add_triangle(model, 1, 0, 2);
    pg_model_add_triangle(model, 1, 2, 3);
}

void pg_model_cube(struct pg_model* model)
{
    pg_model_init(model);
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
                .tex_coord = { (j > 1), (j & 1) } });
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
            .tex_coord = { (j > 1), (j & 1) } });
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
            .tex_coord = { (j > 1), (j & 1) } });
    }
    pg_model_add_triangle(model, idx[0], idx[1], idx[2]);
    pg_model_add_triangle(model, idx[1], idx[3], idx[2]);    
}

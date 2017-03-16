#include <GL/glew.h>
#include "ext/linmath.h"
#include "vertex.h"
#include "viewer.h"
#include "shader.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"
#include "shape.h"

void pg_shape_rect(struct pg_shape* shape)
{
    pg_shape_init(shape);
    pg_shape_add_vertex(shape, &(struct pg_vert2d) {
        .pos = { -0.5, 0.5 }, .tex_coord = { 0, 0 },
        .color = { 0, 0, 0, 255 }, .tex_weight = 0.5 });
    pg_shape_add_vertex(shape, &(struct pg_vert2d) {
        .pos = { 0.5, 0.5 }, .tex_coord = { 1, 0 },
        .color = { 255, 0, 0, 255 }, .tex_weight = 0.5 });
    pg_shape_add_vertex(shape, &(struct pg_vert2d) {
        .pos = { -0.5, -0.5 }, .tex_coord = { 0, 1 },
        .color = { 0, 255, 0, 255 }, .tex_weight = 0.5 });
    pg_shape_add_vertex(shape, &(struct pg_vert2d) {
        .pos = { 0.5, -0.5 }, .tex_coord = { 1, 1 },
        .color = { 0, 0, 255, 255 }, .tex_weight = 0.5 });
    pg_shape_add_triangle(shape, 2, 1, 0);
    pg_shape_add_triangle(shape, 3, 1, 2);
}

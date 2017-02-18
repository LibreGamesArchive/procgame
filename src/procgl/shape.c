#include <GL/glew.h>
#include "vertex.h"
#include "shader.h"
#include "shape.h"

void pg_shape_init(struct pg_shape* shape)
{
    ARR_INIT(shape->verts);
    ARR_INIT(shape->tris);
    glGenBuffers(1, &shape->verts_gl);
    glGenBuffers(1, &shape->tris_gl);
    glGenVertexArrays(1, &shape->vao);
}

void pg_shape_deinit(struct pg_shape* shape)
{
    ARR_DEINIT(shape->verts);
    ARR_DEINIT(shape->tris);
    glDeleteBuffers(1, &shape->verts_gl);
    glDeleteBuffers(1, &shape->tris_gl);
    glDeleteVertexArrays(1, &shape->vao);
}

void pg_shape_buffer(struct pg_shape* shape, struct pg_shader* shader)
{
    glBindVertexArray(shape->vao);
    glBindBuffer(GL_ARRAY_BUFFER, shape->verts_gl);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape->tris_gl);
    glBufferData(GL_ARRAY_BUFFER, shape->verts.len * sizeof(struct pg_vert2d),
                 shape->verts.data, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, shape->tris.len * sizeof(unsigned),
                 shape->tris.data, GL_STATIC_DRAW);
    pg_shader_buffer_attribs(shader);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void pg_shape_begin(struct pg_shape* shape)
{
    glBindVertexArray(shape->vao);
}

unsigned pg_shape_add_vertex(struct pg_shape* shape, struct pg_vert2d* vert)
{
    ARR_PUSH(shape->verts, *vert);
    return shape->verts.len - 1;
}

void pg_shape_add_triangle(struct pg_shape* shape, unsigned v0,
                               unsigned v1, unsigned v2)
{
    int i = shape->tris.len;
    ARR_RESERVE(shape->tris, i + 3);
    shape->tris.len += 3;
    shape->tris.data[i] = v0;
    shape->tris.data[i + 1] = v1;
    shape->tris.data[i + 2] = v2;
}

void pg_shape_draw(struct pg_shape* shape, struct pg_shader* shader,
                   mat4 transform)
{
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, transform);
    glDrawElements(GL_TRIANGLES, shape->tris.len, GL_UNSIGNED_INT, 0);
}

#include "renderer.h"
#include "texture.h"
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

void pg_shape_buffer(struct pg_shape* shape, struct pg_renderer* rend)
{
    glBindVertexArray(shape->vao);
    glBindBuffer(GL_ARRAY_BUFFER, shape->verts_gl);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape->tris_gl);
    printf("Shape tris: %d\n", shape->tris.len);
    glBufferData(GL_ARRAY_BUFFER, shape->verts.len * sizeof(struct pg_vert2d),
                 shape->verts.data, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, shape->tris.len * sizeof(unsigned),
                 shape->tris.data, GL_STATIC_DRAW);
    glVertexAttribPointer(rend->shader_2d.attrs.pos, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert2d), 0);
    glVertexAttribPointer(rend->shader_2d.attrs.tex_coord, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert2d), (void*)(2 * sizeof(float)));
    glVertexAttribPointer(rend->shader_2d.attrs.color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(struct pg_vert2d), (void*)(4 * sizeof(float)));
    glVertexAttribPointer(rend->shader_2d.attrs.tex_weight, 1, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert2d), (void*)(4 * sizeof(float) + 4 * sizeof(uint8_t)));
    glEnableVertexAttribArray(rend->shader_2d.attrs.pos);
    glEnableVertexAttribArray(rend->shader_2d.attrs.tex_coord);
    glEnableVertexAttribArray(rend->shader_2d.attrs.color);
    glEnableVertexAttribArray(rend->shader_2d.attrs.tex_weight);
}

void pg_shape_begin(struct pg_shape* shape)
{
    glBindVertexArray(shape->vao);
    glBindBuffer(GL_ARRAY_BUFFER, shape->verts_gl);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shape->tris_gl);
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

void pg_shape_texture(struct pg_renderer* rend, struct pg_texture* tex)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    glUniform1i(rend->shader_2d.tex, 0);
}

void pg_shape_draw(struct pg_renderer* rend, struct pg_shape* shape,
                   mat4 transform)
{
    glUniformMatrix4fv(rend->shader_2d.model_matrix, 1, GL_FALSE, *transform);
    glDrawElements(GL_TRIANGLES, shape->tris.len, GL_UNSIGNED_INT, 0);
}

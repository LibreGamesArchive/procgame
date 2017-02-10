#include <stdio.h>
#include <GL/glew.h>
#include "renderer.h"

struct data_2d {
    struct {
        GLint model_matrix, tex_unit;
    } unis;
    struct {
        GLint pos, color, tex_coord, tex_weight;
    } attribs;
};

static void deinit(struct pg_shader* shader)
{
    free(shader->data);
}

static void buffer_attribs(struct pg_shader* shader)
{
    struct data_2d* d = shader->data;
    glVertexAttribPointer(d->attribs.pos, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert2d), 0);
    glVertexAttribPointer(d->attribs.tex_coord, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert2d), (void*)(2 * sizeof(float)));
    glVertexAttribPointer(d->attribs.color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                          sizeof(struct pg_vert2d), (void*)(4 * sizeof(float)));
    glVertexAttribPointer(d->attribs.tex_weight, 1, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert2d), (void*)(4 * sizeof(float) + 4 * sizeof(uint8_t)));
    glEnableVertexAttribArray(d->attribs.pos);
    glEnableVertexAttribArray(d->attribs.tex_coord);
    glEnableVertexAttribArray(d->attribs.color);
    glEnableVertexAttribArray(d->attribs.tex_weight);
}

static void begin(struct pg_shader* shader)
{
    glUseProgram(shader->prog);
}

/*  PUBLIC INTERFACE    */

int pg_shader_2d(struct pg_shader* shader)
{
    int load = pg_shader_load(shader,
                              "src/procgl/2d_vert.glsl",
                              "src/procgl/2d_frag.glsl");
    if(!load) return 0;
    struct data_2d* d = malloc(sizeof(struct data_2d));
    d->unis.model_matrix = glGetUniformLocation(shader->prog, "model_matrix");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->attribs.pos = glGetAttribLocation(shader->prog, "v_position");
    d->attribs.color = glGetAttribLocation(shader->prog, "v_color");
    d->attribs.tex_coord = glGetAttribLocation(shader->prog, "v_tex_coord");
    d->attribs.tex_weight = glGetAttribLocation(shader->prog, "v_tex_weight");
    shader->data = d;
    shader->deinit = deinit;
    shader->buffer_attribs = buffer_attribs;
    shader->begin = begin;
    return 1;
}

void pg_shader_2d_set_transform(struct pg_shader* shader, mat4 transform)
{
    struct data_2d* d = shader->data;
    glUniformMatrix4fv(d->unis.model_matrix, 1, GL_FALSE, *transform);
}

void pg_shader_2d_set_texture(struct pg_shader* shader, GLint tex_unit)
{
    struct data_2d* d = shader->data;
    glUniform1i(d->unis.tex_unit, tex_unit);
}

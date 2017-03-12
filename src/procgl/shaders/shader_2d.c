#include <stdio.h>
#include <GL/glew.h>
#include "procgl/ext/linmath.h"
#include "procgl/vertex.h"
#include "procgl/texture.h"
#include "procgl/viewer.h"
#include "procgl/shader.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/2d.glsl.h"
#endif

struct data_2d {
    struct {
        GLint tex_unit;
    } unis;
    struct {
        GLint pos, color, tex_coord, tex_weight;
    } attribs;
};

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

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
}

/*  PUBLIC INTERFACE    */
int pg_shader_2d(struct pg_shader* shader)
{
#ifdef PROCGL_STATIC_SHADERS
    int load = pg_shader_load_static(shader,
        __2d_vert_glsl, __2d_vert_glsl_len,
        __2d_frag_glsl, __2d_frag_glsl_len);
#else
    int load = pg_shader_load(shader,
                              "src/procgl/shaders/2d_vert.glsl",
                              "src/procgl/shaders/2d_frag.glsl");
#endif
    if(!load) return 0;
    struct data_2d* d = malloc(sizeof(struct data_2d));
    pg_shader_link_matrix(shader, PG_MODEL_MATRIX, "model_matrix");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->attribs.pos = glGetAttribLocation(shader->prog, "v_position");
    d->attribs.color = glGetAttribLocation(shader->prog, "v_color");
    d->attribs.tex_coord = glGetAttribLocation(shader->prog, "v_tex_coord");
    d->attribs.tex_weight = glGetAttribLocation(shader->prog, "v_tex_weight");
    shader->data = d;
    shader->deinit = free;
    shader->buffer_attribs = buffer_attribs;
    shader->begin = begin;
    return 1;
}

void pg_shader_2d_set_texture(struct pg_shader* shader, GLint tex_unit)
{
    struct data_2d* d = shader->data;
    glUniform1i(d->unis.tex_unit, tex_unit);
}

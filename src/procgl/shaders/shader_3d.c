#include <stdio.h>
#include <GL/glew.h>
#include "procgl/ext/linmath.h"
#include "procgl/vertex.h"
#include "procgl/wave.h"
#include "procgl/heightmap.h"
#include "procgl/texture.h"
#include "procgl/viewer.h"
#include "procgl/shader.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/3d.glsl.h"
#endif

struct data_3d {
    int tex_dirty;
    struct {
        GLint tex_unit, norm_unit;
    } state;
    struct {
        GLint tex_unit, norm_unit;
    } unis;
    struct {
        GLint pos, normal, tangent, bitangent, tex_coord;
    } attribs;
};

static void buffer_attribs(struct pg_shader* shader)
{
    struct data_3d* d = shader->data;
    glVertexAttribPointer(d->attribs.pos, 3, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert3d), 0);
    glVertexAttribPointer(d->attribs.normal, 3, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert3d),
                          (void*)(3 * sizeof(float)));
    glVertexAttribPointer(d->attribs.tangent, 3, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert3d),
                          (void*)(6 * sizeof(float)));
    glVertexAttribPointer(d->attribs.bitangent, 3, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert3d),
                          (void*)(9 * sizeof(float)));
    glVertexAttribPointer(d->attribs.tex_coord, 2, GL_FLOAT, GL_FALSE,
                          sizeof(struct pg_vert3d),
                          (void*)(12 * sizeof(float)));
    glEnableVertexAttribArray(d->attribs.pos);
    glEnableVertexAttribArray(d->attribs.normal);
    glEnableVertexAttribArray(d->attribs.tangent);
    glEnableVertexAttribArray(d->attribs.bitangent);
    glEnableVertexAttribArray(d->attribs.tex_coord);
}

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data_3d* d = shader->data;
    /*  Set the matrices    */
    mat4 projview;
    mat4_mul(projview, view->view_matrix, view->proj_matrix);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, view->view_matrix);
    pg_shader_set_matrix(shader, PG_PROJECTION_MATRIX, view->proj_matrix);
    pg_shader_set_matrix(shader, PG_PROJECTIONVIEW_MATRIX, projview);
    /*  Set the uniforms    */
    if(d->tex_dirty) {
        glUniform1i(d->unis.tex_unit, d->state.tex_unit);
        glUniform1i(d->unis.norm_unit, d->state.norm_unit);
        d->tex_dirty = 0;
    }
    /*  Enable depth testing    */
    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
}

/*  PUBLIC INTERFACE    */

int pg_shader_3d(struct pg_shader* shader)
{
#ifdef PROCGL_STATIC_SHADERS
    int load = pg_shader_load_static(shader,
        __3d_vert_glsl, __3d_vert_glsl_len,
        __3d_frag_glsl, __3d_frag_glsl_len);
#else
    int load = pg_shader_load(shader,
                              "src/procgl/shaders/3d_vert.glsl",
                              "src/procgl/shaders/3d_frag.glsl");
#endif
    if(!load) return 0;
    struct data_3d* d = malloc(sizeof(struct data_3d));
    pg_shader_link_matrix(shader, PG_MODEL_MATRIX, "model_matrix");
    pg_shader_link_matrix(shader, PG_NORMAL_MATRIX, "normal_matrix");
    pg_shader_link_matrix(shader, PG_VIEW_MATRIX, "view_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTION_MATRIX, "proj_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTIONVIEW_MATRIX, "projview_matrix");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.norm_unit = glGetUniformLocation(shader->prog, "norm");
    d->attribs.pos = glGetAttribLocation(shader->prog, "v_position");
    d->attribs.normal = glGetAttribLocation(shader->prog, "v_normal");
    d->attribs.tangent = glGetAttribLocation(shader->prog, "v_tangent");
    d->attribs.bitangent = glGetAttribLocation(shader->prog, "v_bitangent");
    d->attribs.tex_coord = glGetAttribLocation(shader->prog, "v_tex_coord");
    d->tex_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->buffer_attribs = buffer_attribs;
    shader->begin = begin;
    return 1;
}

void pg_shader_3d_set_texture(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data_3d* d = shader->data;
    d->state.tex_unit = tex->diffuse_slot;
    d->state.norm_unit = tex->light_slot;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, tex->light_slot);
        d->tex_dirty = 0;
    } else d->tex_dirty = 1;
}

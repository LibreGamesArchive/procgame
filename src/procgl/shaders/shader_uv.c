#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include "procgl/ext/linmath.h"
#include "procgl/arr.h"
#include "procgl/wave.h"
#include "procgl/heightmap.h"
#include "procgl/texture.h"
#include "procgl/viewer.h"
#include "procgl/model.h"
#include "procgl/shader.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/uv.glsl.h"
#endif

struct data_uv {
    int tex_dirty;
    struct {
        GLint tex_unit, norm_unit;
    } state;
    struct {
        GLint tex_unit, norm_unit;
    } unis;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data_uv* d = shader->data;
    /*  Set the matrices    */
    mat4 projview;
    mat4_mul(projview, view->view_matrix, view->proj_matrix);
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

int pg_shader_uv(struct pg_shader* shader)
{
#ifdef PROCGL_STATIC_SHADERS
    int load = pg_shader_load_static(shader,
        __uv_vert_glsl, __uv_vert_glsl_len,
        __uv_frag_glsl, __uv_frag_glsl_len);
#else
    int load = pg_shader_load(shader,
                              "src/procgl/shaders/uv_vert.glsl",
                              "src/procgl/shaders/uv_frag.glsl");
#endif
    if(!load) return 0;
    struct data_uv* d = malloc(sizeof(struct data_uv));
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_POSITION, "v_position");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_NORMAL, "v_normal");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_UV, "v_tex_coord");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.norm_unit = glGetUniformLocation(shader->prog, "norm");
    d->tex_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    shader->components =
        (PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_NORMAL |
         PG_MODEL_COMPONENT_UV);
    return 1;
}

void pg_shader_uv_set_texture(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data_uv* d = shader->data;
    d->state.tex_unit = tex->diffuse_slot;
    d->state.norm_unit = tex->light_slot;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, tex->light_slot);
        d->tex_dirty = 0;
    } else d->tex_dirty = 1;
}


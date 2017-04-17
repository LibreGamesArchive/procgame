#include <stdio.h>
#include <GL/glew.h>
#include "procgl/ext/linmath.h"
#include "procgl/vertex.h"
#include "procgl/wave.h"
#include "procgl/heightmap.h"
#include "procgl/texture.h"
#include "procgl/viewer.h"
#include "procgl/model.h"
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
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_POSITION, "v_position");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_NORMAL, "v_normal");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_TANGENT, "v_tangent");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_BITANGENT, "v_bitangent");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_UV, "v_tex_coord");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.norm_unit = glGetUniformLocation(shader->prog, "norm");
    d->tex_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    shader->components =
        (PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_NORMAL |
         PG_MODEL_COMPONENT_TAN_BITAN | PG_MODEL_COMPONENT_UV);
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

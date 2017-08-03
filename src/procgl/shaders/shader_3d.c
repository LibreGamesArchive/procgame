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
#include "procgl/shaders/shaders.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/3d.glsl.h"
#endif

struct data_3d {
    int unis_dirty;
    struct {
        struct pg_texture* tex;
        vec2 tex_scale, tex_offset;
    } state;
    struct {
        GLint tex_unit, norm_unit;
        GLint tex_scale, tex_offset;
    } unis;
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
    if(d->unis_dirty) {
        glUniform1i(d->unis.tex_unit, d->state.tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, d->state.tex->light_slot);
        glUniform2fv(d->unis.tex_offset, 1, d->state.tex_offset);
        glUniform2fv(d->unis.tex_scale, 1, d->state.tex_scale);
        d->unis_dirty = 0;
    }
    /*  Enable depth testing    */
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
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
    d->unis.tex_scale = glGetUniformLocation(shader->prog, "tex_scale");
    d->unis.tex_offset = glGetUniformLocation(shader->prog, "tex_offset");
    d->unis_dirty = 1;
    vec2_set(d->state.tex_scale, 1, 1);
    vec2_set(d->state.tex_offset, 0, 0);
    d->state.tex = 0;
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
    pg_shader_3d_set_tex_offset(shader, (vec2){ 0, 0 });
    pg_shader_3d_set_tex_scale(shader, (vec2){ 1, 1 });
    d->state.tex = tex;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, tex->light_slot);
        d->unis_dirty = 0;
    } else d->unis_dirty = 1;
}

void pg_shader_3d_set_tex_offset(struct pg_shader* shader, vec2 offset)
{
    struct data_3d* d = shader->data;
    vec2_dup(d->state.tex_offset, offset);
    if(pg_shader_is_active(shader)) {
        glUniform2fv(d->unis.tex_offset, 1, d->state.tex_offset);
    } else d->unis_dirty = 1;
}

void pg_shader_3d_set_tex_scale(struct pg_shader* shader, vec2 scale)
{
    struct data_3d* d = shader->data;
    vec2_dup(d->state.tex_scale, scale);
    if(pg_shader_is_active(shader)) {
        glUniform2fv(d->unis.tex_scale, 1, d->state.tex_scale);
    } else d->unis_dirty = 1;
}

void pg_shader_3d_set_tex_frame(struct pg_shader* shader, int frame)
{
    struct data_3d* d = shader->data;
    int frames_wide = d->state.tex->w / d->state.tex->frame_w;
    float frame_u = (float)d->state.tex->frame_w / d->state.tex->w;
    float frame_v = (float)d->state.tex->frame_h / d->state.tex->h;
    float frame_x = (float)(frame % frames_wide) * frame_u;
    float frame_y = (float)(frame / frames_wide) * frame_v;
    vec2_set(d->state.tex_offset, frame_x, frame_y);
    vec2_set(d->state.tex_scale, frame_u, frame_v);
    if(pg_shader_is_active(shader)) {
        glUniform2fv(d->unis.tex_offset, 1, d->state.tex_offset);
        glUniform2fv(d->unis.tex_scale, 1, d->state.tex_scale);
    } else d->unis_dirty = 1;
}



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
#include "procgl/shaders/cubetex.glsl.h"
#endif

struct data_cubetex {
    int tex_dirty;
    struct {
        GLint tex_front, tex_back, tex_left, tex_right, tex_top, tex_bottom;
        GLint norm_front, norm_back, norm_left, norm_right, norm_top, norm_bottom;
    } state;
    struct {
        GLint tex_front, tex_back, tex_left, tex_right, tex_top, tex_bottom;
        GLint norm_front, norm_back, norm_left, norm_right, norm_top, norm_bottom;
    } unis;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data_cubetex* d = shader->data;
    /*  Set the matrices    */
    mat4 projview;
    mat4_mul(projview, view->view_matrix, view->proj_matrix);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, view->view_matrix);
    pg_shader_set_matrix(shader, PG_PROJECTION_MATRIX, view->proj_matrix);
    pg_shader_set_matrix(shader, PG_PROJECTIONVIEW_MATRIX, projview);
    /*  Set the uniforms    */
    if(d->tex_dirty) {
        glUniform1i(d->unis.tex_front, d->state.tex_front);
        glUniform1i(d->unis.tex_back, d->state.tex_back);
        glUniform1i(d->unis.tex_left, d->state.tex_left);
        glUniform1i(d->unis.tex_right, d->state.tex_right);
        glUniform1i(d->unis.tex_top, d->state.tex_top);
        glUniform1i(d->unis.tex_bottom, d->state.tex_bottom);
        glUniform1i(d->unis.norm_front, d->state.norm_front);
        glUniform1i(d->unis.norm_back, d->state.norm_back);
        glUniform1i(d->unis.norm_left, d->state.norm_left);
        glUniform1i(d->unis.norm_right, d->state.norm_right);
        glUniform1i(d->unis.norm_top, d->state.norm_top);
        glUniform1i(d->unis.norm_bottom, d->state.norm_bottom);
        d->tex_dirty = 0;
    }
    /*  Enable depth testing    */
    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
}

/*  PUBLIC INTERFACE    */

int pg_shader_cubetex(struct pg_shader* shader)
{
#ifdef PROCGL_STATIC_SHADERS
    int load = pg_shader_load_static(shader,
        cubetex_vert_glsl, cubetex_vert_glsl_len,
        cubetex_frag_glsl, cubetex_frag_glsl_len);
#else
    int load = pg_shader_load(shader,
                              "src/procgl/shaders/cubetex_vert.glsl",
                              "src/procgl/shaders/cubetex_frag.glsl");
#endif
    if(!load) return 0;
    struct data_cubetex* d = malloc(sizeof(struct data_cubetex));
    pg_shader_link_matrix(shader, PG_MODEL_MATRIX, "model_matrix");
    pg_shader_link_matrix(shader, PG_NORMAL_MATRIX, "normal_matrix");
    pg_shader_link_matrix(shader, PG_VIEW_MATRIX, "view_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTION_MATRIX, "proj_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTIONVIEW_MATRIX, "projview_matrix");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_POSITION, "v_position");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_NORMAL, "v_normal");
    d->unis.tex_front = glGetUniformLocation(shader->prog, "tex_front");
    d->unis.tex_back = glGetUniformLocation(shader->prog, "tex_back");
    d->unis.tex_left = glGetUniformLocation(shader->prog, "tex_left");
    d->unis.tex_right = glGetUniformLocation(shader->prog, "tex_right");
    d->unis.tex_top = glGetUniformLocation(shader->prog, "tex_top");
    d->unis.tex_bottom = glGetUniformLocation(shader->prog, "tex_bottom");
    d->unis.norm_front = glGetUniformLocation(shader->prog, "norm_front");
    d->unis.norm_back = glGetUniformLocation(shader->prog, "norm_back");
    d->unis.norm_left = glGetUniformLocation(shader->prog, "norm_left");
    d->unis.norm_right = glGetUniformLocation(shader->prog, "norm_right");
    d->unis.norm_top = glGetUniformLocation(shader->prog, "norm_top");
    d->unis.norm_bottom = glGetUniformLocation(shader->prog, "norm_bottom");
    d->tex_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    shader->components =
        (PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_NORMAL);
    return 1;
}

void pg_shader_cubetex_set_texture(struct pg_shader* shader,
                                  struct pg_texture_cube* tex_cube)
{
    struct data_cubetex* d = shader->data;
    d->state.tex_front = tex_cube->tex[0]->diffuse_slot;
    d->state.tex_back = tex_cube->tex[1]->diffuse_slot;
    d->state.tex_left = tex_cube->tex[2]->diffuse_slot;
    d->state.tex_right = tex_cube->tex[3]->diffuse_slot;
    d->state.tex_top = tex_cube->tex[4]->diffuse_slot;
    d->state.tex_bottom = tex_cube->tex[5]->diffuse_slot;
    d->state.norm_front = tex_cube->tex[0]->light_slot;
    d->state.norm_back = tex_cube->tex[1]->light_slot;
    d->state.norm_left = tex_cube->tex[2]->light_slot;
    d->state.norm_right = tex_cube->tex[3]->light_slot;
    d->state.norm_top = tex_cube->tex[4]->light_slot;
    d->state.norm_bottom = tex_cube->tex[5]->light_slot;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_front, d->state.tex_front);
        glUniform1i(d->unis.tex_back, d->state.tex_back);
        glUniform1i(d->unis.tex_left, d->state.tex_left);
        glUniform1i(d->unis.tex_right, d->state.tex_right);
        glUniform1i(d->unis.tex_top, d->state.tex_top);
        glUniform1i(d->unis.tex_bottom, d->state.tex_bottom);
        glUniform1i(d->unis.norm_front, d->state.norm_front);
        glUniform1i(d->unis.norm_back, d->state.norm_back);
        glUniform1i(d->unis.norm_left, d->state.norm_left);
        glUniform1i(d->unis.norm_right, d->state.norm_right);
        glUniform1i(d->unis.norm_top, d->state.norm_top);
        glUniform1i(d->unis.norm_bottom, d->state.norm_bottom);
    }
}


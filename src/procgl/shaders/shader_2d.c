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
#include "procgl/shaders/2d.glsl.h"
#endif

struct data_2d {
    int unis_dirty;
    struct {
        struct pg_texture* tex;
        float tex_weight;
        vec4 tex_tx;
        vec4 color_mod, color_add;
        vec2 light_pos;
        vec3 light_color;
        vec3 ambient_color;
    } state;
    struct {
        GLint tex_unit, norm_unit;
        GLint tex_weight;
        GLint tex_tx;
        GLint color_mod, color_add;
        GLint light_pos, light_color, ambient_color;
    } unis;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data_2d* d = shader->data;
    /*  Set the uniforms    */
    if(d->unis_dirty) {
        glUniform1i(d->unis.tex_unit, d->state.tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, d->state.tex->light_slot);
        if(d->state.tex->light_slot >= 0)
            glUniform1i(d->unis.norm_unit, d->state.tex->light_slot);
        else glUniform1i(d->unis.norm_unit, d->state.tex->diffuse_slot);
        glUniform1f(d->unis.tex_weight, d->state.tex_weight);
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
        glUniform4fv(d->unis.color_mod, 1, d->state.color_mod);
        glUniform4fv(d->unis.color_add, 1, d->state.color_add);
        glUniform2fv(d->unis.light_pos, 1, d->state.light_pos);
        glUniform3fv(d->unis.light_color, 1, d->state.light_color);
        glUniform3fv(d->unis.ambient_color, 1, d->state.ambient_color);
        d->unis_dirty = 0;
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
                              SHADER_BASE_DIR "2d_vert.glsl",
                              SHADER_BASE_DIR "2d_frag.glsl");
#endif
    if(!load) return 0;
    struct data_2d* d = malloc(sizeof(struct data_2d));
    pg_shader_link_matrix(shader, PG_MODEL_MATRIX, "model_matrix");
    pg_shader_link_matrix(shader, PG_VIEW_MATRIX, "view_matrix");
    pg_shader_link_matrix(shader, PG_NORMAL_MATRIX, "normal_matrix");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_POSITION, "v_position");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_UV, "v_tex_coord");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_COLOR, "v_color");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_HEIGHT, "v_tex_weight");
    d->state.tex = NULL;
    d->state.tex_weight = 1;
    vec4_set(d->state.color_mod, 1, 1, 1, 1);
    vec4_set(d->state.tex_tx, 1, 1, 0, 0);
    vec2_set(d->state.light_pos, 0, 1);
    vec3_set(d->state.light_color, 0.25, 0, 0);
    vec3_set(d->state.ambient_color, 1, 1, 1);
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.norm_unit = glGetUniformLocation(shader->prog, "norm");
    d->unis.tex_weight = glGetUniformLocation(shader->prog, "tex_weight");
    d->unis.tex_tx = glGetUniformLocation(shader->prog, "tex_tx");
    d->unis.color_mod = glGetUniformLocation(shader->prog, "color_mod");
    d->unis.color_add = glGetUniformLocation(shader->prog, "color_add");
    d->unis.light_pos = glGetUniformLocation(shader->prog, "light_pos");
    d->unis.light_color = glGetUniformLocation(shader->prog, "light_color");
    d->unis.ambient_color = glGetUniformLocation(shader->prog, "ambient_color");
    d->unis_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    shader->components =
        (PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV |
         PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    return 1;
}

void pg_shader_2d_resolution(struct pg_shader* shader, vec2 const resolution)
{
    mat4 tx;
    mat4_ortho(tx, 0, resolution[0], resolution[1], 0, 0, 1);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, tx);
}

void pg_shader_2d_ndc(struct pg_shader* shader, vec2 const scale)
{
    mat4 tx;
    mat4_ortho(tx, -scale[0], scale[0], scale[1], -scale[1], 0, 1);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, tx);
}

void pg_shader_2d_transform(struct pg_shader* shader, vec2 const pos, vec2 const size,
                            float rotation)
{
    mat4 tx;
    mat4_translate(tx, pos[0], pos[1], 0);
    mat4_rotate_Z(tx, tx, rotation);
    mat4_scale_aniso(tx, tx, size[0], size[1], 1);
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, tx);
    mat4 norm;
    mat4_identity(norm);
    mat4_rotate_Z(norm, norm, rotation);
    pg_shader_set_matrix(shader, PG_NORMAL_MATRIX, norm);
}

void pg_shader_2d_texture(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data_2d* d = shader->data;
    d->state.tex = tex;
    pg_shader_2d_tex_transform(shader, (vec2){ 1, 1 }, (vec2){});
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex->diffuse_slot);
        if(tex->light_slot >= 0) glUniform1i(d->unis.norm_unit, tex->light_slot);
        else glUniform1i(d->unis.norm_unit, tex->diffuse_slot);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_tex_weight(struct pg_shader* shader, float weight)
{
    struct data_2d* d = shader->data;
    d->state.tex_weight = weight;
    if(pg_shader_is_active(shader)) {
        glUniform1f(d->unis.tex_weight, d->state.tex_weight);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_tex_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset)
{
    struct data_2d* d = shader->data;
    vec4_set(d->state.tex_tx, scale[0], scale[1], offset[0], offset[1]);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_add_tex_tx(struct pg_shader* shader, vec2 const scale, vec2 const offset)
{
    struct data_2d* d = shader->data;
    vec2_mul(d->state.tex_tx, d->state.tex_tx, scale);
    vec2_add(d->state.tex_tx + 2, d->state.tex_tx + 2, offset);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_tex_frame(struct pg_shader* shader, int frame)
{
    struct data_2d* d = shader->data;
    int frames_wide = d->state.tex->w / d->state.tex->frame_w;
    float frame_u = (float)d->state.tex->frame_w / d->state.tex->w;
    float frame_v = (float)d->state.tex->frame_h / d->state.tex->h;
    float frame_x = (float)(frame % frames_wide) * frame_u;
    float frame_y = (float)(frame / frames_wide) * frame_v;
    vec4_set(d->state.tex_tx, frame_u, frame_v, frame_x, frame_y);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_color_mod(struct pg_shader* shader, vec4 const color, vec4 const add)
{
    struct data_2d* d = shader->data;
    vec4_dup(d->state.color_mod, color);
    vec4_dup(d->state.color_add, add);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.color_mod, 1, d->state.color_mod);
        glUniform4fv(d->unis.color_add, 1, d->state.color_add);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_set_light(struct pg_shader* shader, vec2 const pos,
                            vec3 const color, vec3 const ambient)
{
    struct data_2d* d = shader->data;
    vec2_dup(d->state.light_pos, pos);
    vec3_dup(d->state.light_color, color);
    vec3_dup(d->state.ambient_color, ambient);
    if(pg_shader_is_active(shader)) {
        glUniform2fv(d->unis.light_pos, 1, d->state.light_pos);
        glUniform3fv(d->unis.light_color, 1, d->state.light_color);
        glUniform3fv(d->unis.ambient_color, 1, d->state.ambient_color);
    } else d->unis_dirty = 1;
}

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
        vec4 color_mod;
    } state;
    struct {
        GLint tex_unit;
        GLint tex_weight;
        GLint tex_tx;
        GLint color_mod;
    } unis;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data_2d* d = shader->data;
    /*  Set the uniforms    */
    if(d->unis_dirty) {
        glUniform1i(d->unis.tex_unit, d->state.tex->diffuse_slot);
        glUniform1f(d->unis.tex_weight, d->state.tex_weight);
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
        glUniform4fv(d->unis.color_mod, 1, d->state.color_mod);
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
                              "src/procgl/shaders/2d_vert.glsl",
                              "src/procgl/shaders/2d_frag.glsl");
#endif
    if(!load) return 0;
    struct data_2d* d = malloc(sizeof(struct data_2d));
    pg_shader_link_matrix(shader, PG_MODELVIEW_MATRIX, "transform");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_POSITION, "v_position");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_UV, "v_tex_coord");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_COLOR, "v_color");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_HEIGHT, "v_tex_weight");
    d->state.tex = NULL;
    d->state.tex_weight = 1;
    vec4_set(d->state.color_mod, 1, 1, 1, 1);
    vec4_set(d->state.tex_tx, 1, 1, 0, 0);
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.tex_weight = glGetUniformLocation(shader->prog, "tex_weight");
    d->unis.tex_tx = glGetUniformLocation(shader->prog, "tex_tx");
    d->unis.color_mod = glGetUniformLocation(shader->prog, "color_mod");
    d->unis_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    shader->components =
        (PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV |
         PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    return 1;
}

void pg_shader_2d_resolution(struct pg_shader* shader, vec2 resolution)
{
    mat4 tx;
    mat4_ortho(tx, 0, resolution[0], resolution[1], 0, 0, 1);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, tx);
}

void pg_shader_2d_ndc(struct pg_shader* shader, vec2 scale)
{
    mat4 tx;
    mat4_ortho(tx, -scale[0], scale[0], scale[1], -scale[1], 0, 1);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, tx);
}

void pg_shader_2d_transform(struct pg_shader* shader, vec2 pos, vec2 size,
                            float rotation)
{
    struct data_2d* d = shader->data;
    mat4 tx;
    mat4_translate(tx, pos[0], pos[1], 0);
    mat4_scale_aniso(tx, tx, size[0], size[1], 1);
    mat4_rotate_Z(tx, tx, rotation);
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, tx);
    if(pg_shader_is_active(shader)) {
        pg_shader_rebuild_matrices(shader);
    };
}

void pg_shader_2d_texture(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data_2d* d = shader->data;
    d->state.tex = tex;
    pg_shader_2d_tex_transform(shader, (vec2){ 1, 1 }, (vec2){});
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex->diffuse_slot);
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

void pg_shader_2d_tex_transform(struct pg_shader* shader, vec2 scale, vec2 offset)
{
    struct data_2d* d = shader->data;
    vec4_set(d->state.tex_tx, scale[0], scale[1], offset[0], offset[1]);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_2d_add_tex_tx(struct pg_shader* shader, vec2 scale, vec2 offset)
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

void pg_shader_2d_color_mod(struct pg_shader* shader, vec4 color)
{
    struct data_2d* d = shader->data;
    vec4_dup(d->state.color_mod, color);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.color_mod, 1, d->state.color_mod);
    } else d->unis_dirty = 1;
}

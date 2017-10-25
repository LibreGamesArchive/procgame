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
#include "procgl/shaders/sprite.glsl.h"
#endif

struct data_sprite {
    int unis_dirty;
    struct {
        struct pg_texture* tex;
        vec4 sp_tx, tex_tx;
        vec4 color_mod;
        int mode;
    } state;
    struct {
        GLint tex_unit, norm_unit;
        GLint sp_tx, tex_tx;
        GLint mode;
        GLint color_mod;
    } unis;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data_sprite* d = shader->data;
    /*  Set the matrices    */
    mat4 projview;
    mat4_mul(projview, view->view_matrix, view->proj_matrix);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, view->view_matrix);
    pg_shader_set_matrix(shader, PG_PROJECTION_MATRIX, view->proj_matrix);
    pg_shader_set_matrix(shader, PG_PROJECTIONVIEW_MATRIX, projview);
    /*  Set the uniforms    */
    if(d->unis_dirty) {
        glUniform1i(d->unis.mode, d->state.mode);
        glUniform1i(d->unis.tex_unit, d->state.tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, d->state.tex->light_slot);
        glUniform4fv(d->unis.sp_tx, 1, d->state.sp_tx);
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
        glUniform4fv(d->unis.color_mod, 1, d->state.color_mod);
        d->unis_dirty = 0;
    }
    /*  Enable depth testing    */
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(1);
}

/*  PUBLIC INTERFACE    */

int pg_shader_sprite(struct pg_shader* shader)
{
#ifdef PROCGL_STATIC_SHADERS
    int load = pg_shader_load_static(shader,
        sprite_vert_glsl, sprite_vert_glsl_len,
        sprite_frag_glsl, sprite_frag_glsl_len);
#else
    int load = pg_shader_load(shader,
                              SHADER_BASE_DIR "sprite_vert.glsl",
                              SHADER_BASE_DIR "sprite_frag.glsl");
#endif
    if(!load) return 0;
    struct data_sprite* d = malloc(sizeof(struct data_sprite));
    pg_shader_link_matrix(shader, PG_MODELVIEW_MATRIX, "modelview_matrix");
    pg_shader_link_matrix(shader, PG_VIEW_MATRIX, "view_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTION_MATRIX, "proj_matrix");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_POSITION, "v_position");
    pg_shader_link_component(shader, PG_MODEL_COMPONENT_UV, "v_tex_coord");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.norm_unit = glGetUniformLocation(shader->prog, "norm");
    d->unis.sp_tx = glGetUniformLocation(shader->prog, "sp_tx");
    d->unis.tex_tx = glGetUniformLocation(shader->prog, "tex_tx");
    d->unis.mode = glGetUniformLocation(shader->prog, "mode");
    d->unis.color_mod = glGetUniformLocation(shader->prog, "color_mod");
    d->state.mode = PG_SPRITE_SPHERICAL;
    vec4_set(d->state.sp_tx, 1, 1, 0, 0);
    vec4_set(d->state.tex_tx, 1, 1, 0, 0);
    vec4_set(d->state.color_mod, 1, 1, 1, 1);
    d->unis_dirty = 1;
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    shader->components =
        (PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV);
    return 1;
}

void pg_shader_sprite_mode(struct pg_shader* shader, int mode)
{
    struct data_sprite* d = shader->data;
    d->state.mode = mode;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.mode, mode);
    } else d->unis_dirty = 1;
}

void pg_shader_sprite_texture(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data_sprite* d = shader->data;
    d->state.tex = tex;
    pg_shader_sprite_tex_transform(shader, (vec2){ 1, 1 }, (vec2){ 0 , 0 });
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex->diffuse_slot);
        glUniform1i(d->unis.norm_unit, tex->light_slot);
    } else d->unis_dirty = 1;
}

void pg_shader_sprite_tex_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset)
{
    struct data_sprite* d = shader->data;
    vec2_dup(d->state.tex_tx, scale);
    vec2_dup(d->state.tex_tx + 2, offset);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_sprite_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset)
{
    struct data_sprite* d = shader->data;
    vec2_dup(d->state.sp_tx, scale);
    vec2_dup(d->state.sp_tx + 2, offset);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.sp_tx, 1, d->state.sp_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_sprite_add_tex_tx(struct pg_shader* shader, vec2 const scale, vec2 const offset)
{
    struct data_sprite* d = shader->data;
    vec2_mul(d->state.tex_tx, d->state.tex_tx, scale);
    vec2_add(d->state.tex_tx + 2, d->state.tex_tx + 2, offset);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.tex_tx, 1, d->state.tex_tx);
    } else d->unis_dirty = 1;
}

void pg_shader_sprite_tex_frame(struct pg_shader* shader, int frame)
{
    struct data_sprite* d = shader->data;
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

void pg_shader_sprite_color_mod(struct pg_shader* shader, vec4 const color_mod)
{
    struct data_sprite* d = shader->data;
    vec4_dup(d->state.color_mod, color_mod);
    if(pg_shader_is_active(shader)) {
        glUniform4fv(d->unis.color_mod, 1, color_mod);
    } else d->unis_dirty = 1;
}

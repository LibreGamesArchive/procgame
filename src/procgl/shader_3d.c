#include <stdio.h>
#include <GL/glew.h>
#include "shader.h"
#include "vertex.h"

struct data_3d {
    struct {
        GLint tex_unit, norm_unit;
        GLint ambient_color, sun_dir, sun_color;
        GLint fog_plane, fog_color;
    } unis;
    struct {
        GLint pos, normal, tangent, bitangent, tex_coord;
    } attribs;
    struct {
        GLint tex_unit, norm_unit;
        vec3 ambient_color, fog_color, sun_color, sun_dir;
        vec2 fog_plane;
    } state;
    int tex_dirty;
    int sun_dirty;
    int fog_dirty;
};

static void buffer_attribs(struct pg_shader* shader)
{
    struct data_3d* d = shader->data;
    glVertexAttribPointer(d->attribs.pos, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), 0);
    glVertexAttribPointer(d->attribs.normal, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(d->attribs.tangent, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(6 * sizeof(float)));
    glVertexAttribPointer(d->attribs.bitangent, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(9 * sizeof(float)));
    glVertexAttribPointer(d->attribs.tex_coord, 2, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(12 * sizeof(float)));
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
    if(d->fog_dirty) {
        glUniform2f(d->unis.fog_plane, d->state.fog_plane[0],
                    d->state.fog_plane[1]);
        glUniform3f(d->unis.fog_color, d->state.fog_color[0],
                    d->state.fog_color[1], d->state.fog_color[2]);
        d->fog_dirty = 0;
    }
    if(d->sun_dirty) {
        glUniform3f(d->unis.ambient_color, d->state.ambient_color[0],
                    d->state.ambient_color[1], d->state.ambient_color[2]);
        glUniform3f(d->unis.sun_dir, d->state.sun_dir[0], d->state.sun_dir[1],
                    d->state.sun_dir[2]);
        glUniform3f(d->unis.sun_color, d->state.sun_color[0],
                    d->state.sun_color[1], d->state.sun_color[2]);
        d->sun_dirty = 0;
    }
    /*  Enable depth testing    */
    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
}

/*  PUBLIC INTERFACE    */

int pg_shader_3d(struct pg_shader* shader)
{
    int load = pg_shader_load(shader,
                              "src/procgl/3d_vert.glsl",
                              "src/procgl/3d_frag.glsl");
    if(!load) return 0;
    struct data_3d* d = malloc(sizeof(struct data_3d));
    pg_shader_link_matrix(shader, PG_MODEL_MATRIX, "model_matrix");
    pg_shader_link_matrix(shader, PG_VIEW_MATRIX, "view_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTION_MATRIX, "proj_matrix");
    pg_shader_link_matrix(shader, PG_PROJECTIONVIEW_MATRIX, "projview_matrix");
    d->unis.tex_unit = glGetUniformLocation(shader->prog, "tex");
    d->unis.norm_unit = glGetUniformLocation(shader->prog, "norm");
    d->unis.ambient_color = glGetUniformLocation(shader->prog, "ambient_color");
    d->unis.sun_dir = glGetUniformLocation(shader->prog, "sun_dir");
    d->unis.sun_color = glGetUniformLocation(shader->prog, "sun_color");
    d->unis.fog_color = glGetUniformLocation(shader->prog, "fog_color");
    d->unis.fog_plane = glGetUniformLocation(shader->prog, "fog_plane");
    d->attribs.pos = glGetAttribLocation(shader->prog, "v_position");
    d->attribs.normal = glGetAttribLocation(shader->prog, "v_normal");
    d->attribs.tangent = glGetAttribLocation(shader->prog, "v_tangent");
    d->attribs.bitangent = glGetAttribLocation(shader->prog, "v_bitangent");
    d->attribs.tex_coord = glGetAttribLocation(shader->prog, "tex_coord");
    shader->data = d;
    shader->deinit = free;
    shader->buffer_attribs = buffer_attribs;
    shader->begin = begin;
    return 1;
}

void pg_shader_3d_set_texture(struct pg_shader* shader,
                                 GLint tex_unit, GLint norm_unit)
{
    struct data_3d* d = shader->data;
    d->state.tex_unit = tex_unit;
    d->state.norm_unit = norm_unit;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->unis.tex_unit, tex_unit);
        glUniform1i(d->unis.norm_unit, norm_unit);
        d->tex_dirty = 0;
    } else d->fog_dirty = 1;
}

void pg_shader_3d_set_light(struct pg_shader* shader, vec3 ambient_color,
                               vec3 sun_dir, vec3 sun_color)
{
    struct data_3d* d = shader->data;
    if(ambient_color) vec3_dup(d->state.ambient_color, ambient_color);
    if(sun_color) vec3_dup(d->state.sun_color, sun_color);
    if(sun_dir) vec3_dup(d->state.sun_dir, sun_dir);
    if(pg_shader_is_active(shader)) {
        glUniform3f(d->unis.ambient_color,
                    ambient_color[0], ambient_color[1], ambient_color[2]);
        glUniform3f(d->unis.sun_dir, sun_dir[0], sun_dir[1], sun_dir[2]);
        glUniform3f(d->unis.sun_color, sun_color[0], sun_color[1], sun_color[2]);
        d->sun_dirty = 0;
    } else d->sun_dirty = 1;
}

void pg_shader_3d_set_fog(struct pg_shader* shader, vec2 fog_plane,
                             vec3 fog_color)
{
    struct data_3d* d = shader->data;
    if(fog_plane) vec3_dup(d->state.fog_color, fog_color);
    if(fog_color) vec2_dup(d->state.fog_plane, fog_plane);
    if(pg_shader_is_active(shader)) {
        glUniform2f(d->unis.fog_plane, fog_plane[0], fog_plane[1]);
        glUniform3f(d->unis.fog_color, fog_color[0], fog_color[1], fog_color[2]);
        d->fog_dirty = 0;
    } d->fog_dirty = 1;
}

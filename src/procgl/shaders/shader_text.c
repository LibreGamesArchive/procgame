#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "procgl/procgl_base.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/text.glsl.h"
#endif

struct data {
    GLuint dummy_vao;
    struct pg_texture* font;
    GLint uni_normal, uni_3d;
    GLint uni_font, uni_pitch, uni_glyph;
    GLint uni_blocks, uni_style, uni_color;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data* d = shader->data;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
    glUniform1i(d->uni_font, d->font->diffuse_slot);
    glUniform1ui(d->uni_pitch, d->font->w / d->font->frame_w);
    glUniform2f(d->uni_glyph,
                (float)d->font->frame_w / (float)d->font->w,
                (float)d->font->frame_h / (float)d->font->h);
    glBindVertexArray(d->dummy_vao);
}

/*  PUBLIC INTERFACE    */
int pg_shader_text(struct pg_shader* shader)
{
#ifdef PROCGL_STATIC_SHADERS
    int load = pg_shader_load_static(shader,
                                     text_vert_glsl, text_vert_glsl_len,
                                     text_frag_glsl, text_frag_glsl_len);
#else
    int load = pg_shader_load(shader,
                              "src/procgl/shaders/text_vert.glsl",
                              "src/procgl/shaders/text_frag.glsl");
#endif
    if(!load) return 0;
    struct data* d = malloc(sizeof(struct data));
    pg_shader_link_matrix(shader, PG_MODELVIEW_MATRIX, "transform");
    d->uni_normal = glGetUniformLocation(shader->prog, "normal_3d");
    d->uni_3d = glGetUniformLocation(shader->prog, "is_3d");
    d->uni_font = glGetUniformLocation(shader->prog, "font");
    d->uni_pitch = glGetUniformLocation(shader->prog, "font_pitch");
    d->uni_glyph = glGetUniformLocation(shader->prog, "glyph_size");
    d->uni_blocks = glGetUniformLocation(shader->prog, "chars");
    d->uni_style = glGetUniformLocation(shader->prog, "style");
    d->uni_color = glGetUniformLocation(shader->prog, "color");
    glGenVertexArrays(1, &d->dummy_vao);
    shader->data = d;
    shader->deinit = free;
    shader->begin = begin;
    return 1;
}

void pg_shader_text_3d(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data* d = shader->data;
    glUniform1i(d->uni_3d, 1);
    /*  Set the matrices    */
    mat4 projview;
    mat4_mul(projview, view->proj_matrix, view->view_matrix);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, projview);
    /*  Enable depth testing    */
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(1);
}


void pg_shader_text_resolution(struct pg_shader* shader, vec2 const resolution)
{
    struct data* d = shader->data;
    glUniform1i(d->uni_3d, 0);
    mat4 tx;
    mat4_ortho(tx, 0, resolution[0], resolution[1], 0, 0, 1);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, tx);
}

void pg_shader_text_ndc(struct pg_shader* shader, vec2 const scale)
{
    struct data* d = shader->data;
    glUniform1i(d->uni_3d, 0);
    mat4 tx;
    mat4_ortho(tx, -scale[0], scale[0], scale[1], -scale[1], 0, 1);
    pg_shader_set_matrix(shader, PG_VIEW_MATRIX, tx);
}

void pg_shader_text_transform_3d(struct pg_shader* shader, mat4 tx)
{
    struct data* d = shader->data;
    glUniform3f(d->uni_normal, tx[0][2], tx[1][2], tx[2][2]);
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, tx);
    pg_shader_rebuild_matrices(shader);
}

void pg_shader_text_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset)
{
    mat4 tx;
    mat4_translate(tx, offset[0], offset[1], 0);
    mat4_scale_aniso(tx, tx, scale[0], scale[1], 1);
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, tx);
    pg_shader_rebuild_matrices(shader);
}

void pg_shader_text_font(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data* d = shader->data;
    d->font = tex;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->uni_font, tex->diffuse_slot);
        glUniform1ui(d->uni_pitch, tex->w / tex->frame_w);
        glUniform2f(d->uni_glyph, (float)tex->frame_w / (float)tex->w,
                                  (float)tex->frame_h / (float)tex->h);
    }
}

void pg_shader_text_write(struct pg_shader* shader, struct pg_shader_text* text)
{
    struct data* d = shader->data;
    glUniform1uiv(d->uni_blocks, 16 * text->use_blocks, (uint32_t*)text->block);
    glUniform4fv(d->uni_style, text->use_blocks, text->block_style[0]);
    glUniform4fv(d->uni_color, text->use_blocks, text->block_color[0]);
    glDrawArrays(GL_TRIANGLES, 0, text->use_blocks * 64 * 6);
}

#include <stdio.h>
#include <GL/glew.h>
#include "../procgl/procgl.h"

struct data {
    GLuint dummy_vao;
    struct pg_texture* font;
    GLint font_uni, font_pitch;
    GLint screen_size, glyph_size, char_size;
    GLint start_pos, spacing, string;
};

static void begin(struct pg_shader* shader, struct pg_viewer* view)
{
    struct data* d = shader->data;
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
    glUniform1i(d->font_uni, d->font->color_slot);
    glUniform1ui(d->font_pitch, d->font->w / d->font->frame_w);
    glUniform2f(d->glyph_size,
                (float)d->font->frame_w / (float)d->font->w,
                (float)d->font->frame_h / (float)d->font->h);
    glUniform2f(d->screen_size, view->size[0], view->size[1]);
    glBindVertexArray(d->dummy_vao);
}

/*  PUBLIC INTERFACE    */
int pg_shader_text(struct pg_shader* shader)
{
    int load = pg_shader_load(shader,
                              "shaders/text_vert.glsl",
                              "shaders/text_frag.glsl");
    if(!load) return 0;
    struct data* d = malloc(sizeof(struct data));
    d->font_uni = glGetUniformLocation(shader->prog, "font");
    d->font_pitch = glGetUniformLocation(shader->prog, "font_pitch");
    d->screen_size = glGetUniformLocation(shader->prog, "screen_size");
    d->glyph_size = glGetUniformLocation(shader->prog, "glyph_size");
    d->char_size = glGetUniformLocation(shader->prog, "char_size");
    d->start_pos = glGetUniformLocation(shader->prog, "start_pos");
    d->spacing = glGetUniformLocation(shader->prog, "spacing");
    d->string = glGetUniformLocation(shader->prog, "string");
    glGenVertexArrays(1, &d->dummy_vao);
    shader->data = d;
    shader->deinit = free;
    shader->buffer_attribs = NULL;
    shader->begin = begin;
    return 1;
}

void pg_shader_text_set_font(struct pg_shader* shader, struct pg_texture* tex)
{
    struct data* d = shader->data;
    d->font = tex;
    if(pg_shader_is_active(shader)) {
        glUniform1i(d->font_uni, d->font->color_slot);
        glUniform1i(d->font_pitch, d->font->w / d->font->frame_w);
        glUniform2f(d->glyph_size,
                    d->font->frame_w / d->font->w,
                    d->font->frame_h / d->font->h);
    }
}

void pg_shader_text_write(struct pg_shader* shader, const char* str,
                          vec2 start, vec2 size, float space)
{
    struct data* d = shader->data;
    uint32_t packed_str[32] = {};
    size_t len = strnlen(str, 128);
    strncpy((char*)(&packed_str[0]), str, 128);
    glUniform1uiv(d->string, 32, packed_str);
    glUniform2f(d->start_pos, start[0], start[1]);
    glUniform2f(d->char_size, size[0], size[1]);
    glUniform1f(d->spacing, space);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, len);
}

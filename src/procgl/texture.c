/*
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>

#include "ext/noise1234.h"
#include "ext/linmath.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"*/

#include "procgl.h"
#include "ext/lodepng.h"

void pg_texture_init_from_file(struct pg_texture* tex, const char* file)
{
    unsigned w, h;
    lodepng_decode32_file((uint8_t**)&tex->data, &w, &h, file);
    tex->type = PG_UBYTE;
    tex->channels = 4;
    tex->w = w;
    tex->h = h;
    tex->frame_w = w;
    tex->frame_h = h;
    tex->frame_aspect_ratio = (float)w / (float)h;
    glGenTextures(1, &tex->handle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, tex->w, tex->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, tex->data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

static const struct tex_format {
    int channels;
    enum pg_data_type pgtype;
    GLenum iformat, pixformat, type;
    size_t size;
} gl_tex_formats[] = {
    [PG_UBYTE] = { 1, PG_UBYTE, GL_R8, GL_RED, GL_UNSIGNED_BYTE, sizeof(uint8_t) },
    [PG_UBVEC2] = { 2, PG_UBYTE, GL_RG8, GL_RG, GL_UNSIGNED_BYTE, sizeof(uint8_t) * 2 },
    [PG_UBVEC3] = { 3, PG_UBYTE, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE, sizeof(uint8_t) * 3 },
    [PG_UBVEC4] = { 4, PG_UBYTE, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, sizeof(uint8_t) * 4 },
    [PG_UINT] = { 1, PG_UINT, GL_R32UI, GL_RED, GL_UNSIGNED_INT, sizeof(uint32_t) },
    [PG_UVEC2] = { 2, PG_UINT, GL_RG32UI, GL_RG, GL_UNSIGNED_INT, sizeof(uint32_t) * 2 },
    [PG_UVEC3] = { 3, PG_UINT, GL_RGB32UI, GL_RGB, GL_UNSIGNED_INT, sizeof(uint32_t) * 3 },
    [PG_UVEC4] = { 4, PG_UINT, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT, sizeof(uint32_t) * 4 },
    [PG_INT] = { 1, PG_INT, GL_R32I, GL_RED, GL_INT, sizeof(int32_t) },
    [PG_IVEC2] = { 2, PG_INT, GL_RG32I, GL_RG, GL_INT, sizeof(int32_t) * 2 },
    [PG_IVEC3] = { 3, PG_INT, GL_RGB32I, GL_RGB, GL_INT, sizeof(int32_t) * 3 },
    [PG_IVEC4] = { 4, PG_INT, GL_RGBA32I, GL_RGBA, GL_INT, sizeof(int32_t) * 4 },
    [PG_FLOAT] = { 1, PG_FLOAT, GL_R32F, GL_RED, GL_FLOAT, sizeof(float) },
    [PG_VEC2] = { 2, PG_FLOAT, GL_RG32F, GL_RG, GL_FLOAT, sizeof(float) * 2 },
    [PG_VEC3] = { 3, PG_FLOAT, GL_RGB32F, GL_RGB, GL_FLOAT, sizeof(float) * 3 },
    [PG_VEC4] = { 4, PG_FLOAT, GL_RGBA32F, GL_RGBA, GL_FLOAT, sizeof(float) * 4  }
};

void pg_texture_init(struct pg_texture* tex, int w, int h,
                     enum pg_data_type type)
{
    if(type < PG_UBYTE || type >= PG_MATRIX) {
        *tex = (struct pg_texture){};
        return;
    }
    const struct tex_format* fmt = &gl_tex_formats[type];
    tex->type = fmt->pgtype;
    tex->channels = fmt->channels;
    tex->w = w;
    tex->h = h;
    tex->frame_w = w;
    tex->frame_h = h;
    tex->frame_aspect_ratio = w / h;
    tex->data = calloc(w * h, fmt->size);
    glGenTextures(1, &tex->handle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt->iformat, w, h, 0,
                 fmt->pixformat, fmt->type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
    
void pg_texture_deinit(struct pg_texture* tex)
{
    if(!tex->data) return;
    glDeleteTextures(1, &tex->handle);
    free(tex->data);
}

void pg_texture_bind(struct pg_texture* tex, int slot)
{
    if(tex->type == PG_NULL) return;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, tex->handle);
}

void pg_texture_buffer(struct pg_texture* tex)
{
    if(tex->type == PG_NULL) return;
    enum pg_data_type fulltype = tex->type + tex->channels - 1;
    const struct tex_format* fmt = &gl_tex_formats[fulltype];
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt->iformat, tex->w, tex->h, 0,
                 fmt->pixformat, fmt->type, tex->data);
}

void pg_texture_set_atlas(struct pg_texture* tex, int frame_w, int frame_h)
{
    tex->frame_w = frame_w;
    tex->frame_h = frame_h;
    tex->frame_aspect_ratio = (float)frame_w / (float)frame_h;
}

void pg_texture_get_frame(struct pg_texture* tex, int frame, vec4 out)
{
    int frames_wide = tex->w / tex->frame_w;
    float frame_u = (float)tex->frame_w / tex->w;
    float frame_v = (float)tex->frame_h / tex->h;
    float frame_x = (float)(frame % frames_wide) * frame_u;
    float frame_y = (float)(frame / frames_wide) * frame_v;
    vec4_set(out, frame_x, frame_y, frame_x + frame_u, frame_y + frame_v);
}

void pg_texture_frame_flip(vec4 out, vec4 const in, int x, int y)
{
    vec4 tmp = { in[0], in[1], in[2], in[3] };
    if(x) {
        tmp[0] = in[2];
        tmp[2] = in[0];
    }
    if(y) {
        tmp[1] = in[3];
        tmp[3] = in[1];
    }
    vec4_dup(out, tmp);
}

void pg_texture_frame_tx(vec4 out, vec4 const in,
                         vec2 const scale, vec2 const offset)
{
    vec4 tmp = { in[0], in[1], in[2], in[3] };
    vec2 diff;
    vec2_sub(diff, in + 2, in);
    vec2_mul(diff, diff, scale);
    vec2_add(tmp + 2, tmp, vec2(diff[0], diff[1]));
    vec2_add(tmp, tmp, offset);
    vec2_add(tmp + 2, tmp + 2, offset);
    vec4_dup(out, tmp);
}

void pg_renderbuffer_init(struct pg_renderbuffer* buffer)
{
    *buffer = (struct pg_renderbuffer){};
    glGenFramebuffers(1, &buffer->fbo);
}

void pg_renderbuffer_attach(struct pg_renderbuffer* buffer,
                            struct pg_texture* tex,
                            int idx, GLenum attachment)
{
    if(attachment == GL_DEPTH_ATTACHMENT
    || attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        buffer->depth = tex;
        buffer->depth_attachment = tex ? attachment : GL_NONE;
    } else {
        buffer->outputs[idx] = tex;
        buffer->attachments[idx] = tex ? attachment : GL_NONE;
    }
    buffer->dirty = 1;
}

void pg_renderbuffer_dst(struct pg_renderbuffer* buffer)
{
    glBindFramebuffer(GL_FRAMEBUFFER, buffer->fbo);
    if(buffer->dirty) {
        int w = -1, h = -1;
        buffer->dirty = 0;
        int i;
        for(i = 0; i < 8; ++i) {
            if(!buffer->outputs[i]) continue;
            if(w < 0) w = buffer->outputs[i]->w;
            else w = MIN(w, buffer->outputs[i]->w);
            if(h < 0) h = buffer->outputs[i]->h;
            else h = MIN(h, buffer->outputs[i]->h);
            glFramebufferTexture2D(GL_FRAMEBUFFER, buffer->attachments[i],
                                   GL_TEXTURE_2D, buffer->outputs[i]->handle, 0);
        }
        if(buffer->depth) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, buffer->depth_attachment,
                                   GL_TEXTURE_2D, buffer->depth->handle, 0);
        }
        buffer->w = w;
        buffer->h = h;
    }
    glDrawBuffers(8, buffer->attachments);
    glViewport(0, 0, buffer->w, buffer->h);
}

void pg_rendertarget_init(struct pg_rendertarget* target,
                          struct pg_renderbuffer* b0, struct pg_renderbuffer* b1)
{
    target->buf[0] = b0;
    target->buf[1] = b1;
    target->cur_dst = 0;
}

void pg_rendertarget_dst(struct pg_rendertarget* target)
{
    pg_renderbuffer_dst(target->buf[target->cur_dst]);
}

void pg_rendertarget_swap(struct pg_rendertarget* target)
{
    if(!target->buf[1]) return;
    target->cur_dst = 1 - target->cur_dst;
}

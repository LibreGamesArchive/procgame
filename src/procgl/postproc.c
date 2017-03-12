#include <stdio.h>
#include <GL/glew.h>
#include "ext/linmath.h"
#include "postproc.h"
#include "viewer.h"
#include "shader.h"

void pg_ppbuffer_init(struct pg_ppbuffer* buf, int w, int h,
                      int color0, int color1, int depth0, int depth1)
{
    buf->w = w;
    buf->h = h;
    buf->dst = 0;
    buf->color_unit[0] = color0;
    buf->color_unit[1] = color1;
    buf->depth_unit[0] = depth0;
    buf->depth_unit[1] = depth1;
    glGenTextures(2, buf->color);
    glGenTextures(2, buf->depth);
    glGenFramebuffers(2, buf->frame);
    glActiveTexture(GL_TEXTURE0 + color0);
    glBindTexture(GL_TEXTURE_2D, buf->color[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glActiveTexture(GL_TEXTURE0 + color1);
    glBindTexture(GL_TEXTURE_2D, buf->color[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glActiveTexture(GL_TEXTURE0 + depth0);
    glBindTexture(GL_TEXTURE_2D, buf->depth[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glActiveTexture(GL_TEXTURE0 + depth1);
    glBindTexture(GL_TEXTURE_2D, buf->depth[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0,
                 GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, buf->frame[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, buf->color[0], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D, buf->depth[0], 0); 
    glBindFramebuffer(GL_FRAMEBUFFER, buf->frame[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, buf->color[1], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                           GL_TEXTURE_2D, buf->depth[1], 0); 
}

void pg_ppbuffer_deinit(struct pg_ppbuffer* buf)
{
    glDeleteTextures(2, buf->color);
    glDeleteTextures(2, buf->depth);
    glDeleteFramebuffers(2, buf->frame);
}

void pg_ppbuffer_set_units(struct pg_ppbuffer* buf,
                           GLenum color0, GLenum color1,
                           GLenum depth0, GLenum depth1)
{
    buf->color_unit[0] = color0;
    buf->color_unit[1] = color1;
    buf->depth_unit[0] = depth0;
    buf->depth_unit[1] = depth1;
}

void pg_ppbuffer_bind_all(struct pg_ppbuffer* buf)
{
    glActiveTexture(GL_TEXTURE0 + buf->color_unit[0]);
    glBindTexture(GL_TEXTURE_2D, buf->color[0]);
    glActiveTexture(GL_TEXTURE0 + buf->color_unit[1]);
    glBindTexture(GL_TEXTURE_2D, buf->color[1]);
    glActiveTexture(GL_TEXTURE0 + buf->depth_unit[0]);
    glBindTexture(GL_TEXTURE_2D, buf->depth[0]);
    glActiveTexture(GL_TEXTURE0 + buf->depth_unit[1]);
    glBindTexture(GL_TEXTURE_2D, buf->depth[1]);
}

void pg_ppbuffer_bind_active(struct pg_ppbuffer* buf)
{
    glActiveTexture(GL_TEXTURE0 + buf->color_unit[!buf->dst]);
    glBindTexture(GL_TEXTURE_2D, buf->color[!buf->dst]);
    glActiveTexture(GL_TEXTURE0 + buf->depth_unit[!buf->dst]);
    glBindTexture(GL_TEXTURE_2D, buf->depth[!buf->dst]);
}

void pg_ppbuffer_dst(struct pg_ppbuffer* buf)
{
    glBindFramebuffer(GL_FRAMEBUFFER, buf->frame[buf->dst]);
    glViewport(0, 0, buf->w, buf->h);
}

void pg_ppbuffer_swap(struct pg_ppbuffer* buf)
{
    buf->dst = !buf->dst;
}

static vec2 vert_buf[] = { { -1, -1 }, { 1, -1 }, { -1, 1 }, { 1, 1 } };
static unsigned tri_buf[6] = { 0, 1, 2, 2, 1, 3 };

void pg_postproc_load(struct pg_postproc* pp, const char* vert_filename,
                      const char* frag_filename, const char* pos_name,
                      const char* color_name, const char* depth_name)
{
    pg_compile_glsl(&pp->vert, &pp->frag, &pp->prog,
                    vert_filename, frag_filename);
    pp->attrib_pos = glGetAttribLocation(pp->prog, pos_name);
    pp->tex_unit = glGetUniformLocation(pp->prog, color_name);
    pp->depth_unit = glGetUniformLocation(pp->prog, depth_name);
    glGenBuffers(1, &pp->vert_buf);
    glGenBuffers(1, &pp->tri_buf);
    glGenVertexArrays(1, &pp->vao);
    glBindVertexArray(pp->vao);
    glBindBuffer(GL_ARRAY_BUFFER, pp->vert_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vert_buf), vert_buf, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pp->tri_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(tri_buf), tri_buf, GL_STATIC_DRAW);
    glVertexAttribPointer(pp->attrib_pos, 2, GL_FLOAT, GL_FALSE,
                          2 * sizeof(float), 0);
    glEnableVertexAttribArray(pp->attrib_pos);
    glBindVertexArray(0);
    pp->data = NULL;
    pp->deinit = NULL;
    pp->pre = NULL;
}

void pg_postproc_deinit(struct pg_postproc* pp)
{
    glDeleteShader(pp->vert);
    glDeleteShader(pp->frag);
    glDeleteProgram(pp->prog);
    glDeleteBuffers(1, &pp->vert_buf);
    glDeleteBuffers(1, &pp->tri_buf);
    glDeleteVertexArrays(1, &pp->vao);
    if(pp->deinit) pp->deinit(pp->data);
}

void pg_postproc_apply(struct pg_postproc* pp, struct pg_ppbuffer* src,
                       struct pg_ppbuffer* dst)
{
    glUseProgram(pp->prog);
    if(pp->pre) pp->pre(pp);
    if(pp->tex_unit != -1) {
        glUniform1i(pp->tex_unit, src->color_unit[!src->dst]);
    }
    if(pp->depth_unit != -1) {
        glUniform1i(pp->depth_unit, src->depth_unit[!src->dst]);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, dst ? dst->frame[dst->dst] : 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindVertexArray(pp->vao);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

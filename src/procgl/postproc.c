#include <stdio.h>
#include <GL/glew.h>
#include "ext/linmath.h"
#include "arr.h"
#include "postproc.h"
#include "viewer.h"
#include "model.h"
#include "shader.h"

void pg_ppbuffer_init(struct pg_ppbuffer* buf, int w, int h,
                      int color0, int color1)
{
    buf->w = w;
    buf->h = h;
    buf->dst = 0;
    buf->color_unit[0] = color0;
    buf->color_unit[1] = color1;
    glGenTextures(2, buf->color);
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
    glBindFramebuffer(GL_FRAMEBUFFER, buf->frame[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, buf->color[0], 0);
    glBindFramebuffer(GL_FRAMEBUFFER, buf->frame[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, buf->color[1], 0);
}

void pg_ppbuffer_deinit(struct pg_ppbuffer* buf)
{
    glDeleteTextures(2, buf->color);
    glDeleteFramebuffers(2, buf->frame);
}

void pg_ppbuffer_set_units(struct pg_ppbuffer* buf,
                           GLenum color0, GLenum color1)
{
    buf->color_unit[0] = color0;
    buf->color_unit[1] = color1;
}

void pg_ppbuffer_bind_all(struct pg_ppbuffer* buf)
{
    glActiveTexture(GL_TEXTURE0 + buf->color_unit[0]);
    glBindTexture(GL_TEXTURE_2D, buf->color[0]);
    glActiveTexture(GL_TEXTURE0 + buf->color_unit[1]);
    glBindTexture(GL_TEXTURE_2D, buf->color[1]);
}

void pg_ppbuffer_bind_active(struct pg_ppbuffer* buf)
{
    glActiveTexture(GL_TEXTURE0 + buf->color_unit[!buf->dst]);
    glBindTexture(GL_TEXTURE_2D, buf->color[!buf->dst]);
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

void pg_ppbuffer_swapdst(struct pg_ppbuffer* buf)
{
    buf->dst = !buf->dst;
    glBindFramebuffer(GL_FRAMEBUFFER, buf->frame[buf->dst]);
    glViewport(0, 0, buf->w, buf->h);
}

void pg_postproc_load(struct pg_postproc* pp,
                      const char* vert_filename, const char* frag_filename,
                      const char* color_name, const char* size_name)
{
    pg_compile_glsl(&pp->vert, &pp->frag, &pp->prog,
                    vert_filename, frag_filename);
    glGenVertexArrays(1, &pp->dummy_vao);
    if(color_name) pp->tex_unit = glGetUniformLocation(pp->prog, color_name);
    else pp->tex_unit = -1;
    if(size_name) pp->uni_size = glGetUniformLocation(pp->prog, size_name);
    else pp->uni_size = -1;
    pp->data = NULL;
    pp->deinit = NULL;
    pp->pre = NULL;
}

void pg_postproc_load_static(struct pg_postproc* pp,
                      const char* vert, int vert_len,
                      const char* frag, int frag_len,
                      const char* color_name, const char* size_name)
{
    pg_compile_glsl_static(&pp->vert, &pp->frag, &pp->prog,
                           vert, vert_len, frag, frag_len);
    glGenVertexArrays(1, &pp->dummy_vao);
    if(color_name) pp->tex_unit = glGetUniformLocation(pp->prog, color_name);
    else pp->tex_unit = -1;
    if(size_name) pp->uni_size = glGetUniformLocation(pp->prog, size_name);
    else pp->uni_size = -1;
    pp->data = NULL;
    pp->deinit = NULL;
    pp->pre = NULL;
}

void pg_postproc_deinit(struct pg_postproc* pp)
{
    glDeleteShader(pp->vert);
    glDeleteShader(pp->frag);
    glDeleteProgram(pp->prog);
    glDeleteVertexArrays(1, &pp->dummy_vao);
    if(pp->deinit) pp->deinit(pp->data);
}

void pg_postproc_apply(struct pg_postproc* pp, struct pg_ppbuffer* src)
{
    glUseProgram(pp->prog);
    if(pp->pre) pp->pre(pp);
    if(pp->tex_unit != -1) {
        glUniform1i(pp->tex_unit, src->color_unit[!src->dst]);
    }
    if(pp->uni_size != -1) {
        glUniform2fv(pp->uni_size, 1, (vec2){ src->w, src->h });
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindVertexArray(pp->dummy_vao);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/screen.glsl.h"
#endif

void pg_postproc_screen(struct pg_postproc* pp)
{
#ifdef PROCGL_STATIC_SHADERS
    pg_postproc_load_static(pp, screen_vert_glsl, screen_vert_glsl_len,
                            screen_frag_glsl, screen_frag_glsl_len,
                            "color", "resolution");
#else
    pg_postproc_load(pp, SHADER_BASE_DIR "screen_vert.glsl",
                         SHADER_BASE_DIR "post_screen_frag.glsl",
                         "color", "resolution");
#endif
}


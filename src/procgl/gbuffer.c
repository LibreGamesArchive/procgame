#include <stdio.h>
#include <GL/glew.h>
#include "procgl.h"

void pg_gbuffer_init(struct pg_gbuffer* gbuf, int w, int h)
{
    gbuf->w = w;
    gbuf->h = h;
    /*  Set up the G-buffer, light accumulation buffer, depth buffer,
        and the framebuffer that they are all attached to   */
    glGenTextures(1, &gbuf->color);
    glGenTextures(1, &gbuf->normal);
    glGenTextures(1, &gbuf->pos);
    glGenTextures(1, &gbuf->light);
    glGenRenderbuffers(1, &gbuf->depth);
    glGenFramebuffers(1, &gbuf->frame);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuf->color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, gbuf->normal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, gbuf->pos);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, gbuf->light);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindRenderbuffer(GL_RENDERBUFFER, gbuf->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    /*  Attach all the buffers. 0, 1, 2, and the depth buffer are all drawn
        in the geometry pass. 3 is the light accumulation buffer and is only
        drawn in the lighting pass; each light's pixels are additively
        blended into it, then drawn on top of the color buffer at the end   */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, gbuf->color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D, gbuf->normal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
                           GL_TEXTURE_2D, gbuf->pos, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3,
                           GL_TEXTURE_2D, gbuf->light, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, gbuf->depth);
    /*  Load the shader to render the light volumes */
    pg_compile_glsl(&gbuf->l_vert, &gbuf->l_frag, &gbuf->l_prog,
                    "shaders/deferred_vert.glsl", "shaders/deferred_frag.glsl");
    gbuf->uni_projview = glGetUniformLocation(gbuf->l_prog, "projview_matrix");
    gbuf->uni_normal = glGetUniformLocation(gbuf->l_prog, "g_normal");
    gbuf->uni_pos = glGetUniformLocation(gbuf->l_prog, "g_pos");
    gbuf->uni_light = glGetUniformLocation(gbuf->l_prog, "light");
    gbuf->uni_color = glGetUniformLocation(gbuf->l_prog, "color");
    /*  Load the shader which combines the light accumulation buffer and the
        color buffer, and draws the final result    */
    pg_compile_glsl(&gbuf->f_vert, &gbuf->f_frag, &gbuf->f_prog,
                    "shaders/screen_vert.glsl", "shaders/screen_frag.glsl");
    gbuf->f_color = glGetUniformLocation(gbuf->f_prog, "color");
    gbuf->f_light = glGetUniformLocation(gbuf->f_prog, "light");
    gbuf->f_ambient = glGetUniformLocation(gbuf->f_prog, "ambient_light");
    glGenVertexArrays(1, &gbuf->dummy_vao);
}

void pg_gbuffer_deinit(struct pg_gbuffer* gbuf)
{
    glDeleteTextures(1, &gbuf->color);
    glDeleteTextures(1, &gbuf->normal);
    glDeleteTextures(1, &gbuf->pos);
    glDeleteTextures(1, &gbuf->light);
    glDeleteRenderbuffers(1, &gbuf->depth);
    glDeleteFramebuffers(1, &gbuf->frame);
    glDeleteVertexArrays(1, &gbuf->dummy_vao);
    glDeleteShader(gbuf->l_vert);
    glDeleteShader(gbuf->l_frag);
    glDeleteProgram(gbuf->l_prog);
    glDeleteShader(gbuf->f_vert);
    glDeleteShader(gbuf->f_frag);
    glDeleteProgram(gbuf->f_prog);
}

void pg_gbuffer_bind(struct pg_gbuffer* gbuf, int color_slot,
                     int normal_slot, int pos_slot, int light_slot)
{
    gbuf->color_slot = color_slot;
    gbuf->normal_slot = normal_slot;
    gbuf->pos_slot = pos_slot;
    gbuf->light_slot = light_slot;
    glActiveTexture(GL_TEXTURE0 + color_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->color);
    glActiveTexture(GL_TEXTURE0 + normal_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->normal);
    glActiveTexture(GL_TEXTURE0 + pos_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->pos);
    glActiveTexture(GL_TEXTURE0 + light_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->light);
}

static GLenum drawbufs[4] =
    { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
      GL_COLOR_ATTACHMENT3 };

void pg_gbuffer_dst(struct pg_gbuffer* gbuf)
{
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    glViewport(0, 0, gbuf->w, gbuf->h);
    glDrawBuffers(3, drawbufs);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void pg_gbuffer_begin_light(struct pg_gbuffer* gbuf, struct pg_viewer* view)
{
    /*  Set the output buffer to the light accumulation buffer  */
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    glDrawBuffers(1, drawbufs + 3);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(0);
    /*  All the lights are just added on top of each other  */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glUseProgram(gbuf->l_prog);
    /*  It only needs to know positions and normals for each pixel  */
    glUniform1i(gbuf->uni_normal, gbuf->normal_slot);
    glUniform1i(gbuf->uni_pos, gbuf->pos_slot);
    mat4 projview;
    mat4_mul(projview, view->proj_matrix, view->view_matrix);
    glUniformMatrix4fv(gbuf->uni_projview, 1, GL_FALSE, *projview);
    /*  A dummy VAO because the light volume mesh is defined in the shader  */
    glBindVertexArray(gbuf->dummy_vao);
}

void pg_gbuffer_draw_light(struct pg_gbuffer* gbuf, vec4 light, vec3 color)
{
    /*  Just pass the light (x, y, z, radius), and the color (rgb), then
        draw the light volume mesh  */
    glUniform4fv(gbuf->uni_light, 1, light);
    glUniform3fv(gbuf->uni_color, 1, color);
    glDrawArrays(GL_TRIANGLES, 0, 60);
}

void pg_gbuffer_finish(struct pg_gbuffer* gbuf, vec3 ambient_light)
{
    /*  The lighting pass is done so we should stop blending    */
    glDisable(GL_BLEND);
    /*  Passing the color buffer and the light accumulation buffer, and the
        desired ambient light level, to the shader  */
    glUseProgram(gbuf->f_prog);
    glUniform1i(gbuf->f_color, gbuf->color_slot);
    glUniform1i(gbuf->f_light, gbuf->light_slot);
    glUniform3fv(gbuf->f_ambient, 1, ambient_light);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    /*  Unbind and reset everything back to normal  */
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(1);
}

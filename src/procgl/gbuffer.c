#include "procgl.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/deferred.glsl.h"
#endif

void pg_light_pointlight(struct pg_light* light, vec3 const pos, float size,
                         vec3 const color)
{
    *light = (struct pg_light){
        .pos = { pos[0], pos[1], pos[2] },
        .color = { color[0], color[1], color[2] },
        .size = size };
}

void pg_light_spotlight(struct pg_light* light, vec3 const pos, float size,
                        vec3 const color, vec3 const dir, float angle)
{
    quat_vec3_to_vec3(light->dir_quat, (vec3){ 0, 0, -1 }, dir);
    vec3_dup(light->pos, pos);
    vec3_dup(light->color, color);
    light->size = size;
    light->angle = angle;
}

void pg_gbuffer_init(struct pg_gbuffer* gbuf, int w, int h)
{
    gbuf->w = w;
    gbuf->h = h;
    /*  Set up the G-buffer, light accumulation buffer, depth buffer,
        and the framebuffer that they are all attached to   */
    glGenTextures(1, &gbuf->color);
    glGenTextures(1, &gbuf->normal);
    glGenTextures(1, &gbuf->light);
    glGenTextures(1, &gbuf->depth);
    glGenFramebuffers(1, &gbuf->frame);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gbuf->color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, gbuf->normal);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, gbuf->light);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0,
                 GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, gbuf->depth);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    /*  Attach all the buffers. 0, 1, 2, and the depth buffer are all drawn
        in the geometry pass. 3 is the light accumulation buffer and is only
        drawn in the lighting pass; each light's pixels are additively
        blended into it, then drawn on top of the color buffer at the end   */
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, gbuf->color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D, gbuf->normal, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, gbuf->depth, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
                           GL_TEXTURE_2D, gbuf->light, 0);
    /*  Load the shader to render the light volumes */
#ifdef PROCGL_STATIC_SHADERS
    pg_compile_glsl_static(&gbuf->l_vert, &gbuf->l_frag, &gbuf->l_prog,
        deferred_vert_glsl, deferred_vert_glsl_len,
        deferred_frag_glsl, deferred_frag_glsl_len);
#else
    pg_compile_glsl(&gbuf->l_vert, &gbuf->l_frag, &gbuf->l_prog,
                    SHADER_BASE_DIR "deferred_vert.glsl",
                    SHADER_BASE_DIR "deferred_frag.glsl");
#endif
    gbuf->uni_projview = glGetUniformLocation(gbuf->l_prog, "projview_matrix");
    gbuf->uni_view = glGetUniformLocation(gbuf->l_prog, "view_matrix");
    gbuf->uni_eye_pos = glGetUniformLocation(gbuf->l_prog, "eye_pos");
    gbuf->uni_normal = glGetUniformLocation(gbuf->l_prog, "g_normal");
    gbuf->uni_depth = glGetUniformLocation(gbuf->l_prog, "g_depth");
    gbuf->uni_light = glGetUniformLocation(gbuf->l_prog, "light");
    gbuf->uni_color = glGetUniformLocation(gbuf->l_prog, "color");
    gbuf->uni_clip = glGetUniformLocation(gbuf->l_prog, "clip_planes");
    /*  And again for spotlights    */
#ifdef PROCGL_STATIC_SHADERS
    pg_compile_glsl_static(&gbuf->spot_vert, &gbuf->spot_frag, &gbuf->spot_prog,
        deferred_spot_vert_glsl, deferred_spot_vert_glsl_len,
        deferred_spot_frag_glsl, deferred_spot_frag_glsl_len);
#else
    pg_compile_glsl(&gbuf->spot_vert, &gbuf->spot_frag, &gbuf->spot_prog,
                    SHADER_BASE_DIR "deferred_spot_vert.glsl",
                    SHADER_BASE_DIR "deferred_spot_frag.glsl");
#endif
    gbuf->uni_projview_spot = glGetUniformLocation(gbuf->spot_prog, "projview_matrix");
    gbuf->uni_view_spot = glGetUniformLocation(gbuf->spot_prog, "view_matrix");
    gbuf->uni_eye_pos_spot = glGetUniformLocation(gbuf->spot_prog, "eye_pos");
    gbuf->uni_normal_spot = glGetUniformLocation(gbuf->spot_prog, "g_normal");
    gbuf->uni_depth_spot = glGetUniformLocation(gbuf->spot_prog, "g_depth");
    gbuf->uni_light_spot = glGetUniformLocation(gbuf->spot_prog, "light");
    gbuf->uni_dir_spot = glGetUniformLocation(gbuf->spot_prog, "dir_quat");
    gbuf->uni_color_spot = glGetUniformLocation(gbuf->spot_prog, "cone_and_color");
    gbuf->uni_clip_spot = glGetUniformLocation(gbuf->spot_prog, "clip_planes");
#if 0
    /*  And again for light groups  */
#ifdef PROCGL_STATIC_SHADERS
    pg_compile_glsl_static(&gbuf->grp_vert, &gbuf->grp_frag, &gbuf->grp_prog,
        deferred_group_spot_vert_glsl, deferred_group_spot_vert_glsl_len,
        deferred_group_spot_frag_glsl, deferred_group_spot_frag_glsl_len);
#else
    pg_compile_glsl(&gbuf->grp_vert, &gbuf->grp_frag, &gbuf->grp_prog,
                    "src/procgl/shaders/deferred_group_spot_vert.glsl",
                    "src/procgl/shaders/deferred_group_spot_frag.glsl");
#endif
    gbuf->uni_projview_grp = glGetUniformLocation(gbuf->grp_prog, "projview_matrix");
    gbuf->uni_view_grp = glGetUniformLocation(gbuf->grp_prog, "view_matrix");
    gbuf->uni_eye_pos_grp = glGetUniformLocation(gbuf->grp_prog, "eye_pos");
    gbuf->uni_normal_grp = glGetUniformLocation(gbuf->grp_prog, "g_normal");
    gbuf->uni_depth_grp = glGetUniformLocation(gbuf->grp_prog, "g_depth");
    gbuf->uni_light_grp = glGetUniformLocation(gbuf->grp_prog, "light");
    gbuf->uni_dir_grp = glGetUniformLocation(gbuf->grp_prog, "dir_angle");
    gbuf->uni_color_grp = glGetUniformLocation(gbuf->grp_prog, "color");
    gbuf->uni_clip_grp = glGetUniformLocation(gbuf->grp_prog, "clip_planes");
    gbuf->uni_group_pos = glGetUniformLocation(gbuf->grp_prog, "group_pos");
    gbuf->uni_group_bound = glGetUniformLocation(gbuf->grp_prog, "group_bound");
    /*  Load the shader which combines the light accumulation buffer and the
        color buffer, and draws the final result    */
#endif
#ifdef PROCGL_STATIC_SHADERS
    pg_compile_glsl_static(&gbuf->f_vert, &gbuf->f_frag, &gbuf->f_prog,
        screen_vert_glsl, screen_vert_glsl_len,
        screen_frag_glsl, screen_frag_glsl_len);
#else
    pg_compile_glsl(&gbuf->f_vert, &gbuf->f_frag, &gbuf->f_prog,
                    SHADER_BASE_DIR "screen_vert.glsl",
                    SHADER_BASE_DIR "screen_frag.glsl");
#endif
    gbuf->f_color = glGetUniformLocation(gbuf->f_prog, "color");
    gbuf->f_light = glGetUniformLocation(gbuf->f_prog, "light");
    gbuf->f_norm = glGetUniformLocation(gbuf->f_prog, "norm");
    gbuf->f_ambient = glGetUniformLocation(gbuf->f_prog, "ambient_light");
    glGenVertexArrays(1, &gbuf->dummy_vao);
}

void pg_gbuffer_deinit(struct pg_gbuffer* gbuf)
{
    glDeleteTextures(1, &gbuf->color);
    glDeleteTextures(1, &gbuf->normal);
    glDeleteTextures(1, &gbuf->depth);
    glDeleteTextures(1, &gbuf->light);
    glDeleteFramebuffers(1, &gbuf->frame);
    glDeleteVertexArrays(1, &gbuf->dummy_vao);
    glDeleteShader(gbuf->l_vert);
    glDeleteShader(gbuf->l_frag);
    glDeleteProgram(gbuf->l_prog);
    glDeleteShader(gbuf->spot_vert);
    glDeleteShader(gbuf->spot_frag);
    glDeleteProgram(gbuf->spot_prog);
    glDeleteShader(gbuf->f_vert);
    glDeleteShader(gbuf->f_frag);
    glDeleteProgram(gbuf->f_prog);
}

void pg_gbuffer_bind(struct pg_gbuffer* gbuf, int color_slot,
                     int normal_slot, int depth_slot, int light_slot)
{
    gbuf->color_slot = color_slot;
    gbuf->normal_slot = normal_slot;
    gbuf->depth_slot = depth_slot;
    gbuf->light_slot = light_slot;
    glActiveTexture(GL_TEXTURE0 + color_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->color);
    glActiveTexture(GL_TEXTURE0 + normal_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->normal);
    glActiveTexture(GL_TEXTURE0 + depth_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->depth);
    glActiveTexture(GL_TEXTURE0 + light_slot);
    glBindTexture(GL_TEXTURE_2D, gbuf->light);
}

static GLenum drawbufs[3] =
    { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

void pg_gbuffer_dst(struct pg_gbuffer* gbuf)
{
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    glViewport(0, 0, gbuf->w, gbuf->h);
    glDrawBuffers(3, drawbufs);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void pg_gbuffer_begin_pointlight(struct pg_gbuffer* gbuf, struct pg_viewer* view)
{
    /*  Set the output buffer to the light accumulation buffer  */
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    glDrawBuffers(1, drawbufs + 2);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(0);
    glFrontFace(GL_CCW);
    /*  All the lights are just added on top of each other  */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glUseProgram(gbuf->l_prog);
    /*  It only needs to know positions and normals for each pixel  */
    glUniform1i(gbuf->uni_normal, gbuf->normal_slot);
    glUniform1i(gbuf->uni_depth, gbuf->depth_slot);
    mat4 projview;
    mat4_mul(projview, view->proj_matrix, view->view_matrix);
    glUniformMatrix4fv(gbuf->uni_projview, 1, GL_FALSE, *projview);
    glUniformMatrix4fv(gbuf->uni_view, 1, GL_FALSE, *view->view_matrix);
    glUniform3f(gbuf->uni_eye_pos, view->pos[0], view->pos[1], view->pos[2]);
    vec2 clip = { view->near_far[1] / (view->near_far[1] - view->near_far[0]),
                  (-view->near_far[1] * view->near_far[0]) / (view->near_far[1] - view->near_far[0]) };
    glUniform2fv(gbuf->uni_clip, 1, clip);
    /*  A dummy VAO because the light volume mesh is defined in the shader  */
    glBindVertexArray(gbuf->dummy_vao);
}

void pg_gbuffer_begin_spotlight(struct pg_gbuffer* gbuf, struct pg_viewer* view)
{
    /*  Set the output buffer to the light accumulation buffer  */
    glBindFramebuffer(GL_FRAMEBUFFER, gbuf->frame);
    glDrawBuffers(1, drawbufs + 2);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(0);
    glFrontFace(GL_CCW);
    /*  All the lights are just added on top of each other  */
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glUseProgram(gbuf->spot_prog);
    /*  It only needs to know positions and normals for each pixel  */
    glUniform1i(gbuf->uni_normal_spot, gbuf->normal_slot);
    glUniform1i(gbuf->uni_depth_spot, gbuf->depth_slot);
    mat4 projview;
    mat4_mul(projview, view->proj_matrix, view->view_matrix);
    glUniformMatrix4fv(gbuf->uni_projview_spot, 1, GL_FALSE, *projview);
    glUniformMatrix4fv(gbuf->uni_view_spot, 1, GL_FALSE, *view->view_matrix);
    glUniform3f(gbuf->uni_eye_pos_spot, view->pos[0], view->pos[1], view->pos[2]);
    vec2 clip = { view->near_far[1] / (view->near_far[1] - view->near_far[0]),
                  (-view->near_far[1] * view->near_far[0]) / (view->near_far[1] - view->near_far[0]) };
    glUniform2fv(gbuf->uni_clip_spot, 1, clip);
    /*  A dummy VAO because the light volume mesh is defined in the shader  */
    glBindVertexArray(gbuf->dummy_vao);
}

void pg_gbuffer_draw_pointlight(struct pg_gbuffer* gbuf, struct pg_light* light)
{
    glUniform4fv(gbuf->uni_light, 1,
        (vec4){ light->pos[0], light->pos[1], light->pos[2], light->size });
    glUniform3fv(gbuf->uni_color, 1, light->color);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void pg_gbuffer_draw_spotlight(struct pg_gbuffer* gbuf, struct pg_light* light)
{
    glUniform4fv(gbuf->uni_light_spot, 1,
        (vec4){ light->pos[0], light->pos[1], light->pos[2], light->size });
    glUniform4fv(gbuf->uni_dir_spot, 1, light->dir_quat);
    glUniform4fv(gbuf->uni_color_spot, 1, 
        (vec4){ light->color[0], light->color[1], light->color[2], light->angle });
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void pg_gbuffer_mode(struct pg_gbuffer* gbuf, int mode)
{
    if(mode) {
        glDepthFunc(GL_GEQUAL);
        glDepthMask(GL_FALSE);
        glFrontFace(GL_CW);
    } else {
        glDepthFunc(GL_LESS);
        glFrontFace(GL_CCW);
    }
}

void pg_gbuffer_finish(struct pg_gbuffer* gbuf, vec3 ambient_light)
{
    /*  The lighting pass is done so we should stop blending    */
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    /*  Passing the color buffer and the light accumulation buffer, and the
        desired ambient light level, to the shader  */
    glUseProgram(gbuf->f_prog);
    glUniform1i(gbuf->f_color, gbuf->color_slot);
    glUniform1i(gbuf->f_light, gbuf->light_slot);
    glUniform1i(gbuf->f_norm, gbuf->normal_slot);
    glUniform3fv(gbuf->f_ambient, 1, ambient_light);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    /*  Unbind and reset everything back to normal  */
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}

#include "procgl.h"

void pg_render_texture_init(struct pg_render_texture* tex, int w, int h,
                            GLenum iformat, GLenum pixformat, GLenum type)
{
    tex->w = w;
    tex->h = h;
    glGenTextures(1, &tex->handle);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, iformat, w, h, 0, pixformat, type, NULL);
}

void pg_renderer_init(struct pg_renderer* rend)
{
    /*  Load the base shaders and populate the shader table with them   */
    struct pg_shader shader;
    ARR_INIT(rend->passes);
    HTABLE_INIT(rend->common_uniforms, 8);
    HTABLE_INIT(rend->shaders, 8);
    int load = pg_shader_load(&shader,
        "src/procgl/shaders/2d_vert.glsl", "src/procgl/shaders/2d_frag.glsl");
    if(!load) printf("Failed to load 2d shader.\n");
    else HTABLE_SET(rend->shaders, "2d", shader);
}

void pg_renderer_deinit(struct pg_renderer* rend)
{
    struct pg_shader* shader;
    ARR_DEINIT(rend->passes);
    HTABLE_DEINIT(rend->common_uniforms);
    HTABLE_ITER iter;
    HTABLE_ITER_BEGIN(rend->shaders, iter);
    while(!HTABLE_ITER_END(rend->shaders, iter)) {
        HTABLE_ITER_NEXT_PTR(rend->shaders, iter, shader);
        pg_shader_deinit(shader);
    }
    HTABLE_DEINIT(rend->shaders);
}

void pg_renderpass_init(struct pg_renderer* rend, struct pg_renderpass* pass,
                         char* shader, struct pg_render_target* target,
                         GLbitfield clear_buffers)
{
    pg_shader_ref_t shader_ref;
    HTABLE_GET_ENTRY(rend->shaders, shader, shader_ref);
    if(!HTABLE_ENTRY_VALID(rend->shaders, shader_ref)) {
        *pass = (struct pg_renderpass){};
        return;
    }
    *pass = (struct pg_renderpass) {
        .shader = shader_ref,
        .target = target,
        .clear_buffers = clear_buffers };
    HTABLE_INIT(pass->uniforms, 8);
}

void pg_renderpass_deinit(struct pg_renderpass* pass)
{
    HTABLE_DEINIT(pass->uniforms);
    ARR_DEINIT(pass->groups);
}

void pg_renderpass_reset(struct pg_renderpass* pass)
{
    pass->target = NULL;
    pass->clear_buffers = 0;
    HTABLE_CLEAR(pass->uniforms);
    ARR_TRUNCATE_CLEAR(pass->groups, 0);
}

void pg_renderpass_uniform(struct pg_renderpass* pass, char* name,
                            enum pg_data_type type, struct pg_uniform* data)
{
    HTABLE_SET(pass->uniforms, name,
        (struct pg_shader_uniform){ .type = type, .data = *data });
}

void pg_rendergroup_init(struct pg_rendergroup* group, struct pg_model* model)
{
    *group = (struct pg_rendergroup){ .model = model };
    HTABLE_INIT(group->common_uniforms, 8);
}

void pg_rendergroup_deinit(struct pg_rendergroup* group)
{
    HTABLE_DEINIT(group->common_uniforms);
    ARR_DEINIT(group->draws);
}

void pg_rendergroup_uniform(struct pg_rendergroup* group, char* name,
                            enum pg_data_type type, struct pg_uniform* data)
{
    HTABLE_SET(group->common_uniforms, name,
        (struct pg_shader_uniform){ .type = type, .data = *data });
}

void pg_rendergroup_add_draw(struct pg_rendergroup* group, int n,
                             struct pg_uniform* draw_unis)
{
    ARR_RESERVE(group->draws, group->draws.len + n);
    memcpy(group->draws.data + group->draws.len, draw_unis, n * sizeof(*draw_unis));
    group->draws.len += n;
}

void pg_renderer_reset(struct pg_renderer* rend)
{
    ARR_TRUNCATE_CLEAR(rend->passes, 0);
}

void pg_rendergroup_model(struct pg_rendergroup* group, struct pg_model* model)
{
    group->model = model;
}

void pg_rendergroup_texture(struct pg_rendergroup* group, int n, ...)
{
    va_list args;
    va_start(args, n);
    int i;
    for(i = 0; i < 8; ++i) {
        if(i >= n) group->tex[i] = NULL;
        else group->tex[i] = va_arg(args, struct pg_texture*);
    }
    va_end(args);
}

void pg_rendergroup_drawformat(struct pg_rendergroup* group,
                               pg_rendergroup_drawfunc_t per_draw, int n, ...)
{
    group->per_draw = per_draw;
    va_list args;
    va_start(args, n);
    int i;
    for(i = 0; i < 8 && i < n; ++i) {
        char* u = va_arg(args, char*);
        strncpy(group->draw_uniform[i], u, 32);
    }
    va_end(args);
}

static inline void copymats(mat4* dst, mat4* src)
{
    int i;
    for(i = 0; i < PG_COMMON_MATRICES; ++i) {
        mat4_dup(dst[i], src[i]);
    }
}

static inline void shader_base_mats(struct pg_shader* shader, mat4* mats)
{
    struct pg_shader_uniform* uni;
    int i;
    for(i = 0; i < PG_COMMON_MATRICES; ++i) {
        HTABLE_ENTRY_PTR(shader->uniforms, shader->matrix[i], uni);
        if(!uni || uni->idx == -1) return;
        mat4_dup(uni->data.m, mats[i]);
        glUniformMatrix4fv(uni->idx, 1, GL_FALSE, *(mats[i]));
    }
}

/*
static inline void shader_model_mats(struct pg_shader* shader, mat4 mat)
{
    pg_shader_set_matrix(shader, PG_MODEL_MATRIX, mat);
    if(shader->mat_idx[PG_NORMAL_MATRIX] != -1) {
        mat4_invert(shader->matrix[PG_NORMAL_MATRIX],
                    shader->matrix[PG_MODEL_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_NORMAL_MATRIX], 1, GL_TRUE,
                           *shader->matrix[PG_NORMAL_MATRIX]);
    }
    if(shader->mat_idx[PG_MODELVIEW_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_MODELVIEW_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX],
                 shader->matrix[PG_MODEL_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_MODELVIEW_MATRIX], 1, GL_FALSE,
                           *shader->matrix[PG_MODELVIEW_MATRIX]);
    }
    if(shader->mat_idx[PG_PROJECTIONVIEW_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_PROJECTIONVIEW_MATRIX],
                 shader->matrix[PG_PROJECTION_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_PROJECTIONVIEW_MATRIX], 1, GL_FALSE,
                           *shader->matrix[PG_PROJECTIONVIEW_MATRIX]);
    }
    if(shader->mat_idx[PG_MVP_MATRIX] != -1) {
        mat4_mul(shader->matrix[PG_MVP_MATRIX],
                 shader->matrix[PG_MODEL_MATRIX],
                 shader->matrix[PG_VIEW_MATRIX]);
        mat4_mul(shader->matrix[PG_MVP_MATRIX],
                 shader->matrix[PG_PROJECTION_MATRIX],
                 shader->matrix[PG_MVP_MATRIX]);
        glUniformMatrix4fv(shader->mat_idx[PG_MVP_MATRIX], 1, GL_FALSE,
                           *shader->matrix[PG_MVP_MATRIX]);
    }
}*/

static inline void shader_textures(struct pg_shader* shader,
                                   struct pg_rendergroup* group)
{
    struct pg_shader_uniform* uni;
    int i;
    for(i = 0; i < 8; ++i) {
        if(!group->tex[i]) continue;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, group->tex[i]->diffuse_gl);
        HTABLE_ENTRY_PTR(shader->uniforms, shader->tex[i], uni);
        if(!uni || uni->idx == -1) continue;
        uni->data.tex = group->tex[i];
        glUniform1i(uni->idx, i);
    }
}

void pg_renderer_draw_frame(struct pg_renderer* rend)
{
    struct pg_renderpass* pass;
    struct pg_rendergroup* group;
    struct pg_model* model;
    int i, j, k;
    ARR_FOREACH(rend->passes, pass, i) {
        struct pg_shader* shader;
        HTABLE_ENTRY_PTR(rend->shaders, pass->shader, shader);
        if(!shader) continue;
        /*  Render to the current pass' target, or the screen if it's unset */
        if(pass->target) pg_render_target_dst(pass->target);
        else pg_screen_dst();
        /*  Active shader = current pass' shader    */
        pg_shader_begin(shader);
        /*  Set the base matrices from the pass. These will not change with
            each draw, most likely (but if they are, they will change with
            EVERY draw, and it's assumed the user knows what they're doing  */
        shader_base_mats(shader, pass->mats);
        pg_shader_uniforms_from_table(shader, &pass->uniforms);
        GLint uni_idx[8];
        mat4 mats[PG_COMMON_MATRICES];
        ARR_FOREACH(pass->groups, group, j) {
            /*  The scratch-space matrices: these will be the model, modelview,
                and MVP matrices which get changed each draw (by default)   */
            memcpy(mats, pass->mats, sizeof(mat4) * PG_COMMON_MATRICES);
            /*  Group common uniforms   */
            pg_shader_uniforms_from_table(shader, &group->common_uniforms);
            shader_textures(shader, group);
            /*  Bind the group model    */
            pg_model_begin(group->model, shader);
            model = group->model;
            /*  Set up the uniform indices which are passed to per_draw */
            for(k = 0; k < 8; ++k) {
                struct pg_shader_uniform* sh_uni;
                HTABLE_GET(shader->uniforms, group->draw_uniform[k], sh_uni);
                if(!sh_uni) {
                    uni_idx[k] = -1;
                    continue;
                } else uni_idx[k] = sh_uni->idx;
            }
            /*  The inner draw loop */
            k = 0;
            while(k < group->draws.len) {
                k += group->per_draw(group->draws.data + k, shader,
                                     (const mat4*)mats, uni_idx);
                glDrawElements(GL_TRIANGLES, model->tris.len * 3, GL_UNSIGNED_INT, 0);
            }
        }
    }
}

void pg_render_target_init(struct pg_render_target* target)
{
    *target = (struct pg_render_target){};
    glGenFramebuffers(1, &target->fbo);
}

void pg_render_target_attach(struct pg_render_target* target,
                             struct pg_render_texture* tex,
                             int idx, GLenum attachment)
{
    if(attachment == GL_DEPTH_ATTACHMENT
    || attachment == GL_DEPTH_STENCIL_ATTACHMENT) {
        target->depth = tex;
        target->depth_attachment = tex ? attachment : GL_NONE;
    } else {
        target->outputs[idx] = tex;
        target->attachments[idx] = tex ? attachment : GL_NONE;
    }
    target->dirty = 1;
}

void pg_render_target_dst(struct pg_render_target* target)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    int w = -1, h = -1;
    if(target->dirty) {
        target->dirty = 0;
        int i;
        for(i = 0; i < 8; ++i) {
            if(!target->outputs[i]) continue;
            if(w < 0) w = target->outputs[i]->w;
            else w = MIN(w, target->outputs[i]->w);
            if(h < 0) h = target->outputs[i]->h;
            else h = MIN(h, target->outputs[i]->h);
            glFramebufferTexture2D(GL_FRAMEBUFFER, target->attachments[i],
                                   GL_TEXTURE_2D, target->outputs[i]->handle, 0);
        }
        if(target->depth) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, target->depth_attachment,
                                   GL_TEXTURE_2D, target->depth->handle, 0);
        }
    }
    glDrawBuffers(8, target->attachments);
    glViewport(0, 0, w, h);
}

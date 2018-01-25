#include "procgl.h"

void pg_renderer_init(struct pg_renderer* rend)
{
    /*  Load the base shaders and populate the shader table with them   */
    struct pg_shader shader;
    glGenVertexArrays(1, &rend->dummy_vao);
    ARR_INIT(rend->passes);
    HTABLE_INIT(rend->common_uniforms, 8);
    HTABLE_INIT(rend->shaders, 8);
    /*  2d shader   */
    int load = pg_shader_load(&shader,
        "src/procgl/shaders/2d_vert.glsl", "src/procgl/shaders/2d_frag.glsl");
    if(!load) printf("Failed to load 2d shader.\n");
    else HTABLE_SET(rend->shaders, "2d", shader);
    /*  Sine-wave distortion post-effect    */
    load = pg_shader_load(&shader,
        "src/procgl/shaders/screen_vert.glsl", "src/procgl/shaders/post_sine_frag.glsl");
    if(!load) printf("Failed to load sine-wave shader.\n");
    else {
        shader.static_verts = 1;
        shader.is_postproc = 1;
        HTABLE_SET(rend->shaders, "post_sine", shader);
    }
    /*  Blur post-effect    */
    load = pg_shader_load(&shader,
        "src/procgl/shaders/screen_vert.glsl", "src/procgl/shaders/post_blur7_frag.glsl");
    if(!load) printf("Failed to load blur shader.\n");
    else {
        shader.static_verts = 1;
        shader.is_postproc = 1;
        HTABLE_SET(rend->shaders, "post_blur", shader);
    }
    /*  Pass-thru shader    */
    load = pg_shader_load(&shader,
        "src/procgl/shaders/screen_vert.glsl", "src/procgl/shaders/post_screen_frag.glsl");
    if(!load) printf("Failed to load pass-thru shader.\n");
    else {
        shader.static_verts = 1;
        shader.is_postproc = 1;
        HTABLE_SET(rend->shaders, "passthru", shader);
    }
}

void pg_renderer_deinit(struct pg_renderer* rend)
{
    *rend = (struct pg_renderer){};
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
                         char* shader, GLbitfield clear_buffers)
{
    pg_shader_ref_t shader_ref;
    HTABLE_GET_ENTRY(rend->shaders, shader, shader_ref);
    if(!HTABLE_ENTRY_VALID(rend->shaders, shader_ref)) {
        *pass = (struct pg_renderpass){};
        return;
    }
    *pass = (struct pg_renderpass) {
        .shader = shader_ref,
        .clear_buffers = clear_buffers };
    HTABLE_INIT(pass->uniforms, 8);
}

void pg_renderpass_deinit(struct pg_renderpass* pass)
{
    HTABLE_DEINIT(pass->uniforms);
    ARR_DEINIT(pass->draws);
}

void pg_renderpass_reset(struct pg_renderpass* pass)
{
    pass->target = NULL;
    pass->clear_buffers = 0;
    pass->model = NULL;
    int i;
    for(i = 0; i < 8; ++i) {
        pass->tex[i] = NULL;
        pass->draw_uniform[i][0] = '\0';
        pass->per_draw = NULL;
    }
    HTABLE_CLEAR(pass->uniforms);
    ARR_TRUNCATE_CLEAR(pass->draws, 0);
}

void pg_renderpass_uniform(struct pg_renderpass* pass, char* name,
                            enum pg_data_type type, struct pg_uniform* data)
{
    HTABLE_SET(pass->uniforms, name,
        (struct pg_shader_uniform){ .type = type, .data = *data });
}

void pg_renderpass_model(struct pg_renderpass* pass, struct pg_model* model)
{
    pass->model = model;
}

void pg_renderpass_texture(struct pg_renderpass* pass, int n, ...)
{
    va_list args;
    va_start(args, n);
    int i;
    for(i = 0; i < 8; ++i) {
        if(i >= n) pass->tex[i] = NULL;
        else pass->tex[i] = va_arg(args, struct pg_texture*);
    }
    va_end(args);
}

void pg_renderpass_drawformat(struct pg_renderpass* pass,
                               pg_render_drawfunc_t per_draw, int n, ...)
{
    pass->per_draw = per_draw;
    va_list args;
    va_start(args, n);
    int i;
    for(i = 0; i < 8 && i < n; ++i) {
        char* u = va_arg(args, char*);
        strncpy(pass->draw_uniform[i], u, 32);
    }
    va_end(args);
}

void pg_renderpass_add_draw(struct pg_renderpass* pass, int n,
                             struct pg_uniform* draw_unis)
{
    ARR_RESERVE(pass->draws, pass->draws.len + n);
    memcpy(pass->draws.data + pass->draws.len, draw_unis, n * sizeof(*draw_unis));
    pass->draws.len += n;
}

void pg_renderer_reset(struct pg_renderer* rend)
{
    ARR_TRUNCATE_CLEAR(rend->passes, 0);
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

static inline void shader_textures(struct pg_shader* shader, int n,
                                   struct pg_texture** tex_array)
{
    n = MIN(n, 8);
    struct pg_shader_uniform* uni;
    int i;
    for(i = 0; i < n; ++i) {
        if(!tex_array[i]) continue;
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, tex_array[i]->handle);
        HTABLE_ENTRY_PTR(shader->uniforms, shader->tex[i], uni);
        if(!uni || uni->idx == -1) continue;
        uni->data.tex = tex_array[i];
        glUniform1i(uni->idx, i);
    }
}

static inline void shader_fbtextures(struct pg_shader* shader,
                                     struct pg_rendertarget* target)
{
    struct pg_renderbuffer* buf = target->buf[1 - target->cur_dst];
    if(!buf) return;
    struct pg_shader_uniform* uni;
    int i;
    for(i = 0; i < 8; ++i) {
        if(!buf->outputs[i]) continue;
        glActiveTexture(GL_TEXTURE8 + i);
        glBindTexture(GL_TEXTURE_2D, buf->outputs[i]->handle);
        HTABLE_ENTRY_PTR(shader->uniforms, shader->fbtex[i], uni);
        if(!uni || uni->idx == -1) continue;
        uni->data.tex = buf->outputs[i];
        glUniform1i(uni->idx, 8 + i);
    }
    if(buf->depth) {
        glActiveTexture(GL_TEXTURE16);
        glBindTexture(GL_TEXTURE_2D, buf->depth->handle);
        HTABLE_ENTRY_PTR(shader->uniforms, shader->fbdepth, uni);
        if(uni && uni->idx == -1) {
            uni->data.tex = buf->depth;
            glUniform1i(uni->idx, 16);
        }
    }
}

void pg_renderer_draw_frame(struct pg_renderer* rend)
{
    struct pg_shader* lastshader = NULL;
    struct pg_shader* shader;
    struct pg_renderpass* pass = NULL;
    struct pg_model* model;
    int i, j, k;
    ARR_FOREACH(rend->passes, pass, i) {
        HTABLE_ENTRY_PTR(rend->shaders, pass->shader, shader);
        if(!shader) continue;
        if(shader != lastshader) pg_shader_begin(shader);
        lastshader = shader;
        if(pass->model && pass->model != model) pg_model_begin(pass->model, shader);
        else if(shader->static_verts) glBindVertexArray(rend->dummy_vao);
        model = pass->model;
        if(!pass->swaptarget) {
            if(pass->target) pg_rendertarget_dst(pass->target);
            else pg_screen_dst();
            glClear(pass->clear_buffers);
        } else {
        //    pg_rendertarget_swap(pass->target);
        //    pg_rendertarget_dst(pass->target);
        //    shader_fbtextures(shader, pass->target);
        }
        shader_base_mats(shader, pass->mats);
        shader_textures(shader, 8, pass->tex);
        pg_shader_uniforms_from_table(shader, &pass->uniforms);
        GLint uni_idx[8];
        /*  Set up the uniform indices which are passed to per_draw */
        for(j = 0; j < 8; ++j) {
            struct pg_shader_uniform* sh_uni;
            HTABLE_GET(shader->uniforms, pass->draw_uniform[j], sh_uni);
            if(!sh_uni) {
                uni_idx[j] = -1;
                continue;
            } else uni_idx[j] = sh_uni->idx;
        }
        /*  The inner draw loop */
        j = 0;
        while(j < pass->draws.len) {
            if(pass->swaptarget) {
                pg_rendertarget_swap(pass->target);
                pg_rendertarget_dst(pass->target);
                shader_fbtextures(shader, pass->target);
                glClear(pass->clear_buffers);
            }
            j += pass->per_draw(pass->draws.data + j, shader,
                                 (const mat4*)pass->mats, uni_idx);
            if(shader->static_verts) continue;
            glDrawElements(GL_TRIANGLES, model->tris.len * 3, GL_UNSIGNED_INT, 0);
        }
        //if(pass->swaptarget) pg_rendertarget_swap(pass->target);
    }
    if(pass && pass->target) {
        HTABLE_GET(rend->shaders, "passthru", shader);
        pg_shader_begin(shader);
        glBindVertexArray(rend->dummy_vao);
        pg_rendertarget_swap(pass->target);
        shader_fbtextures(shader, pass->target);
        pg_screen_dst();
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

void pg_renderpass_target(struct pg_renderpass* pass,
                          struct pg_rendertarget* target, int swap)
{
    pass->target = target;
    pass->swaptarget = swap;
}


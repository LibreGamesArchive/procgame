#include "htable.h"

struct pg_model;

/*  The basic structure of a render:
    pg_renderer
        pg_renderpass
            pg_render_target                The destination for all draw calls
            pg_rendergroup
                pg_model                    The model and texture to be used
                pg_texture                  for all draws in this bucket.
                array of pg_uniform data
                function pointer to advance through uniform array per draw
*/

struct pg_render_texture {
    GLuint handle;
    int w, h;
};

struct pg_render_target {
    GLuint fbo;
    struct pg_render_texture* outputs[8];
    GLenum attachments[8];
    struct pg_render_texture* depth;
    GLenum depth_attachment;
    int dirty;
};

typedef int (*pg_rendergroup_drawfunc_t)(
                const struct pg_uniform*, const struct pg_shader*,
                const mat4*, const GLint*);

struct pg_rendergroup {
    struct pg_texture* tex[8];
    struct pg_model* model;
    pg_shader_uniform_table_t common_uniforms;
    char draw_uniform[8][32];
    GLint draw_uniform_idx[8];
    ARR_T(struct pg_uniform) draws;
    pg_rendergroup_drawfunc_t per_draw;
};

struct pg_renderpass {
    int in_use;
    pg_shader_ref_t shader;
    struct pg_render_target* target;
    GLbitfield clear_buffers;
    mat4 mats[PG_COMMON_MATRICES];
    pg_shader_uniform_table_t uniforms;
    int ngroups;
    ARR_T(struct pg_rendergroup*) groups;
};

struct pg_renderer {
    pg_shader_table_t shaders;
    pg_shader_uniform_table_t common_uniforms;
    ARR_T(struct pg_renderpass*) passes;
};

/*  Renderer    */
void pg_renderer_init(struct pg_renderer* rend);
void pg_renderer_deinit(struct pg_renderer* rend);
void pg_renderer_draw_frame(struct pg_renderer* rend);
void pg_renderer_buffer_model(struct pg_renderer* rend, char* shader,
                              struct pg_model* model);
void pg_renderer_reset(struct pg_renderer* rend);

/*  Renderpass  */
void pg_renderpass_init(struct pg_renderer* rend, struct pg_renderpass* pass,
                         char* shader, struct pg_render_target* target,
                         GLbitfield clear_buffers);
void pg_renderpass_deinit(struct pg_renderpass* pass);
void pg_renderpass_uniform(struct pg_renderpass* pass, char* name,
                            enum pg_data_type type, struct pg_uniform* data);
void pg_renderpass_reset(struct pg_renderpass* pass);

/*  Rendergroup */
void pg_rendergroup_init(struct pg_rendergroup* group, struct pg_model* model);
void pg_rendergroup_deinit(struct pg_rendergroup* group);
void pg_rendergroup_uniform(struct pg_rendergroup* group, char* name,
                            enum pg_data_type type, struct pg_uniform* data);
void pg_rendergroup_model(struct pg_rendergroup* group, struct pg_model* model);
void pg_rendergroup_texture(struct pg_rendergroup* group, int n, ...);
void pg_rendergroup_drawformat(struct pg_rendergroup* group,
                               pg_rendergroup_drawfunc_t per_draw, int n, ...);
void pg_rendergroup_add_draw(struct pg_rendergroup* group, int n,
                             struct pg_uniform* draw_unis);

/*  Rendertexture   */
void pg_render_texture_init(struct pg_render_texture* tex, int w, int h,
                            GLenum iformat, GLenum pixformat, GLenum type);
void pg_render_target_init(struct pg_render_target* target);
void pg_render_target_attach(struct pg_render_target* target,
                             struct pg_render_texture* tex,
                             int idx, GLenum attachment);
void pg_render_target_dst(struct pg_render_target* target);


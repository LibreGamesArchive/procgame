#include "htable.h"

struct pg_model;

/*  The basic structure of a render:
    pg_renderer
        pg_renderpass
            pg_rendertarget                 The destination for all draw calls
            pg_model                        The model and texture to be used
            pg_texture                      for all draws in this bucket.
            array of pg_uniform data
            draw_func                       Advances through uniform array
*/

typedef int (*pg_render_drawfunc_t)(
                const struct pg_uniform*, const struct pg_shader*,
                const mat4*, const GLint*);

struct pg_renderpass {
    /*  Pass data/options   */
    int disable;
    pg_shader_ref_t shader;
    struct pg_rendertarget* target;
    int swaptarget;
    int finish_to_screen;
    GLbitfield clear_buffers;
    /*  Model to draw with  */
    struct pg_model* model;
    /*  Shader uniforms and matrices    */
    mat4 mats[PG_COMMON_MATRICES];
    pg_shader_uniform_table_t uniforms;
    struct pg_texture* tex[8];
    /*  Raw array of uniform data, processed by drawfunc    */
    char draw_uniform[8][32];
    GLint draw_uniform_idx[8];
    ARR_T(struct pg_uniform) draws;
    pg_render_drawfunc_t per_draw;
};

struct pg_renderer {
    GLuint dummy_vao;
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
                         char* shader, GLbitfield clear_buffers);
void pg_renderpass_deinit(struct pg_renderpass* pass);
void pg_renderpass_reset(struct pg_renderpass* pass);
void pg_renderpass_uniform(struct pg_renderpass* pass, char* name,
                           enum pg_data_type type, struct pg_uniform* data);
void pg_renderpass_target(struct pg_renderpass* pass,
                          struct pg_rendertarget* target, int swap);
void pg_renderpass_model(struct pg_renderpass* pass, struct pg_model* model);
void pg_renderpass_texture(struct pg_renderpass* pass, int n, ...);
void pg_renderpass_drawformat(struct pg_renderpass* pass,
                               pg_render_drawfunc_t per_draw, int n, ...);
void pg_renderpass_add_draw(struct pg_renderpass* pass, int n,
                            struct pg_uniform* draw_unis);


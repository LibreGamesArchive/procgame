#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "../linmath.h"

typedef GLuint buffer_tex_bank[3][4];
typedef GLint shader_tex_bank[3];

struct procgl_renderer {
    /*  Easy view position/angle    */
    vec3 view_pos;
    vec2 view_angle;
    /*  The rest of this points to data/code on the GPU */
    /****************************************************/
    /*  First: UNIFORMS */
    /*  Vertex shader base uniform buffer   */
    GLuint vert_base_buffer;
    struct {
        mat4 proj_matrix;
        mat4 view_matrix;
        mat4 projview_matrix;
    } vert_base;
    /*  Fragment shader base uniform buffer */
    GLuint frag_base_buffer;
    struct {
        vec3 amb_color;
        float pad0;
        vec3 sun_dir;
        float pad1;
        vec3 sun_color;
        float pad2;
        vec2 fog_plane;
        float pad3[2];
        vec3 fog_color;
    } frag_base;
    buffer_tex_bank tex_slots;
    /*  SHADER PROGRAMS */
    /*  The model shader program    */
    struct {
        GLuint vert, frag, prog;
        GLuint model_matrix;
        GLint vert_base_block;
        GLint frag_base_block;
        shader_tex_bank tex_slots;
        struct {
            GLint pos, normal, tangent, bitangent, tex_coord;
        } attrs;
    } shader_model;
    /*  The terrain shader program, with associated terrain-specific
        uniform buffer  */
    struct {
        GLuint vert, frag, prog, data_buffer;
        GLuint model_matrix;
        GLint vert_base_block;
        GLint frag_base_block;
        shader_tex_bank tex_slots;
        GLint data_block;
        struct {
            GLint pos, normal, tangent, bitangent, tex_coord;
        } attrs;
        struct {
            vec4 height_mod;
            vec4 scale;
            vec2 detail_weight;
        } data;
    } shader_terrain;
};

struct vertex {
    vec3 pos, normal, tangent, bitangent;
    vec2 tex_coord;
};

int procgl_renderer_init(struct procgl_renderer* rend,
                  const char* vert_filename, const char* frag_filename);
void procgl_renderer_deinit(struct procgl_renderer* rend);
void procgl_renderer_begin_terrain(struct procgl_renderer* rend);
void procgl_renderer_build_projection(struct procgl_renderer* rend);
void procgl_renderer_begin_model(struct procgl_renderer* rend);
void procgl_renderer_begin_terrain(struct procgl_renderer* rend);

void procgl_renderer_set_view(struct procgl_renderer* rend,
                              vec3 pos, vec2 angle);
void procgl_renderer_set_sun(struct procgl_renderer* rend,
                             vec3 dir, vec3 color, vec3 amb_color);
void procgl_renderer_set_fog(struct procgl_renderer* rend,
                             vec2 plane, vec3 color);

void procgl_renderer_get_view(struct procgl_renderer* rend,
                              vec3 pos, vec2 angle);
void procgl_renderer_get_sun(struct procgl_renderer* rend,
                             vec3 dir, vec3 color, vec3 amb_color);
void procgl_renderer_get_fog(struct procgl_renderer* rend,
                             vec2 plane, vec3 color);

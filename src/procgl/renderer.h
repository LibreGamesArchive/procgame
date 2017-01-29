#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "../linmath.h"
#include "../arr.h"

typedef GLuint buffer_tex_bank[3][4];
typedef GLint shader_tex_bank[3];

struct pg_renderer {
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
    /*  The 2d shader program   */
    struct {
        GLuint vert, frag, prog;
        GLuint model_matrix;
        GLint tex;
        struct {
            GLint pos, color, tex_coord, tex_weight;
        } attrs;
    } shader_2d;
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

struct pg_vert3d {
    vec3 pos, normal, tangent, bitangent;
    vec2 tex_coord;
};

struct pg_vert2d {
    vec2 pos;
    vec2 tex_coord;
    uint8_t color[4];
    float tex_weight;
};

typedef ARR_T(struct pg_vert3d) vert3d_buf_t;
typedef ARR_T(struct pg_vert2d) vert2d_buf_t;
typedef ARR_T(unsigned) tri_buf_t;

int pg_renderer_init(struct pg_renderer* rend,
                  const char* vert_filename, const char* frag_filename);
void pg_renderer_deinit(struct pg_renderer* rend);
void pg_renderer_begin_terrain(struct pg_renderer* rend);
void pg_renderer_build_projection(struct pg_renderer* rend);
void pg_renderer_begin_2d(struct pg_renderer* rend);
void pg_renderer_begin_model(struct pg_renderer* rend);
void pg_renderer_begin_terrain(struct pg_renderer* rend);

void pg_renderer_set_view(struct pg_renderer* rend,
                              vec3 pos, vec2 angle);
void pg_renderer_set_sun(struct pg_renderer* rend,
                             vec3 dir, vec3 color, vec3 amb_color);
void pg_renderer_set_fog(struct pg_renderer* rend,
                             vec2 plane, vec3 color);

void pg_renderer_get_view(struct pg_renderer* rend,
                              vec3 pos, vec2 angle);
void pg_renderer_get_sun(struct pg_renderer* rend,
                             vec3 dir, vec3 color, vec3 amb_color);
void pg_renderer_get_fog(struct pg_renderer* rend,
                             vec2 plane, vec3 color);

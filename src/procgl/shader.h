#include "../linmath.h"
#include "viewer.h"

enum pg_matrix {
    PG_MODEL_MATRIX,
    PG_VIEW_MATRIX,
    PG_PROJECTION_MATRIX,
    PG_MODELVIEW_MATRIX,
    PG_PROJECTIONVIEW_MATRIX,
    PG_MVP_MATRIX
};

struct pg_shader {
    GLuint vert, frag, prog;
    /*  Basically every shader will have some or all of these   */
    GLint mat_idx[6];
    mat4 matrix[6];
    void* data;
    void (*deinit)(void* data);
    void (*buffer_attribs)(struct pg_shader* shader);
    void (*begin)(struct pg_shader* shader, struct pg_viewer* view);
};

/*  Read/compile a shader program and store it in the given shader object   */
int pg_shader_load(struct pg_shader* shader,
                   const char* vert_filename, const char* frag_filename);
/*  Associate a shader's matrix to the name of a shader program uniform */
void pg_shader_link_matrix(struct pg_shader* shader, enum pg_matrix type,
                           const char* name);
/*  Set a shader matrix, and set the corresponding program uniform  */
void pg_shader_set_matrix(struct pg_shader* shader, enum pg_matrix type,
                          mat4 matrix);
/*  Rebuild any of the composite matrices that a shader uses    */
void pg_shader_rebuild_matrices(struct pg_shader* shader);
/*  Check if a shader is currently active   */
int pg_shader_is_active(struct pg_shader* shader);

/*  Per-shader interface    */
void pg_shader_deinit(struct pg_shader* shader);
void pg_shader_buffer_attribs(struct pg_shader* shader);
void pg_shader_begin(struct pg_shader* shader, struct pg_viewer* view);

/*  My shader implementations   */
int pg_shader_2d(struct pg_shader* shader);
void pg_shader_2d_set_texture(struct pg_shader* shader, GLint tex_unit);

int pg_shader_3d(struct pg_shader* shader);
void pg_shader_3d_set_texture(struct pg_shader* shader,
                                 GLint tex_unit, GLint norm_unit);
void pg_shader_3d_set_light(struct pg_shader* shader, vec3 ambient_light,
                               vec3 light_dir, vec3 light_color);
void pg_shader_3d_set_fog(struct pg_shader* shader, vec2 fog_plane,
                             vec3 fog_color);

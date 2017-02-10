#include "../linmath.h"

struct pg_shader {
    GLuint vert, frag, prog;
    void* data;
    void (*deinit)(struct pg_shader* shader);
    void (*buffer_attribs)(struct pg_shader* shader);
    void (*begin)(struct pg_shader* shader);
};

int pg_shader_load(struct pg_shader* shader,
                   const char* vert_filename, const char* frag_filename);
void pg_shader_deinit(struct pg_shader* shader);
void pg_shader_buffer_attribs(struct pg_shader* shader);
void pg_shader_begin(struct pg_shader* shader);

int pg_shader_2d(struct pg_shader* shader);
void pg_shader_2d_set_transform(struct pg_shader* shader, mat4 transform);
void pg_shader_2d_set_texture(struct pg_shader* shader, GLint tex_unit);

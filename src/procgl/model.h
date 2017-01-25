#include "../arr.h"

typedef ARR_T(struct vertex) vert_buf_t;
typedef ARR_T(unsigned) tri_buf_t;

struct procgl_model {
    vert_buf_t verts;
    tri_buf_t tris;
    GLuint verts_gl;
    GLuint tris_gl;
    GLuint vao;
};

void procgl_model_init(struct procgl_model* model);
void procgl_model_deinit(struct procgl_model* model);
void procgl_model_buffer(struct procgl_model* model);
void procgl_model_split_tris(struct procgl_model* model);
void procgl_model_generate_texture(struct procgl_model* model,
                                   struct procgl_texture* texture,
                                   unsigned w, unsigned h);
void procgl_model_precalc_verts(struct procgl_model* model);
void procgl_model_begin(struct procgl_model* model, struct procgl_renderer* rend);
void procgl_model_draw(struct procgl_renderer* rend, struct procgl_model* model,
                       mat4 transform);
void procgl_model_reserve_verts(struct procgl_model* model, unsigned count);
void procgl_model_reserve_tris(struct procgl_model* model, unsigned count);
unsigned procgl_model_add_vertex(struct procgl_model* model, struct vertex* vert);
void procgl_model_add_triangle(struct procgl_model* model, unsigned v0,
                               unsigned v1, unsigned v2);
void procgl_model_append(struct procgl_model* dst, struct procgl_model* src,
                         mat4 transform);
void procgl_model_transform(struct procgl_model* model, mat4 transform);

/*  PRIMITIVES  */
/*  A unit quad centered on the origin, facing +y   */
void procgl_model_quad(struct procgl_model* model);
/*  A unit cube centered on the origin   */
void procgl_model_cube(struct procgl_model* model);
/*  An icosahedron with n subdivisions, positioned on a unit sphere */
void procgl_model_icosphere(struct procgl_model* model, int n);

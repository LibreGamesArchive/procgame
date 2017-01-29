struct pg_model {
    vert3d_buf_t verts;
    tri_buf_t tris;
    GLuint verts_gl;
    GLuint tris_gl;
    GLuint vao;
};

void pg_model_init(struct pg_model* model);
void pg_model_deinit(struct pg_model* model);
void pg_model_split_tris(struct pg_model* model);
void pg_model_generate_texture(struct pg_model* model,
                                   struct pg_texture* texture,
                                   unsigned w, unsigned h);
void pg_model_precalc_verts(struct pg_model* model);
void pg_model_buffer(struct pg_model* model, struct pg_renderer* rend);
void pg_model_begin(struct pg_model* model, struct pg_renderer* rend);
void pg_model_draw(struct pg_renderer* rend, struct pg_model* model,
                       mat4 transform);
void pg_model_reserve_verts(struct pg_model* model, unsigned count);
void pg_model_reserve_tris(struct pg_model* model, unsigned count);
unsigned pg_model_add_vertex(struct pg_model* model, struct pg_vert3d* vert);
void pg_model_add_triangle(struct pg_model* model, unsigned v0,
                               unsigned v1, unsigned v2);
void pg_model_append(struct pg_model* dst, struct pg_model* src,
                         mat4 transform);
void pg_model_transform(struct pg_model* model, mat4 transform);

/*  PRIMITIVES  */
/*  A unit quad centered on the origin, facing +y   */
void pg_model_quad(struct pg_model* model);
/*  A unit cube centered on the origin   */
void pg_model_cube(struct pg_model* model);
/*  An icosahedron with n subdivisions, positioned on a unit sphere */
void pg_model_icosphere(struct pg_model* model, int n);

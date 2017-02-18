struct pg_shape {
    vert2d_buf_t verts;
    tri_buf_t tris;
    GLuint verts_gl;
    GLuint tris_gl;
    GLuint vao;
};

void pg_shape_init(struct pg_shape* shape);
void pg_shape_deinit(struct pg_shape* shape);
void pg_shape_buffer(struct pg_shape* shape, struct pg_shader* shader);
void pg_shape_begin(struct pg_shape* shape);
unsigned pg_shape_add_vertex(struct pg_shape* shape, struct pg_vert2d* vert);
void pg_shape_add_triangle(struct pg_shape* shape, unsigned v0, unsigned v1,
                           unsigned v2);
void pg_shape_draw(struct pg_shape* shape, struct pg_shader* shader,
                   mat4 transform);

void pg_shape_rect(struct pg_shape* shape);

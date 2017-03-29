struct pg_model {
    vert3d_buf_t verts;
    tri_buf_t tris;
    GLuint verts_gl;
    GLuint tris_gl;
    GLuint vao;
};

void pg_model_init(struct pg_model* model);
void pg_model_deinit(struct pg_model* model);
void pg_model_buffer(struct pg_model* model, struct pg_shader* shader);
void pg_model_begin(struct pg_model* model);
void pg_model_draw(struct pg_model* model, struct pg_shader* shader,
                   mat4 transform);
void pg_model_reserve_verts(struct pg_model* model, unsigned count);
void pg_model_reserve_tris(struct pg_model* model, unsigned count);
unsigned pg_model_add_vertex(struct pg_model* model, struct pg_vert3d* vert);
void pg_model_add_triangle(struct pg_model* model, unsigned v0,
                               unsigned v1, unsigned v2);
void pg_model_append(struct pg_model* dst, struct pg_model* src,
                         mat4 transform);
void pg_model_transform(struct pg_model* model, mat4 transform);
void pg_model_generate_texture(struct pg_model* model,
                               struct pg_texture* texture,
                               unsigned w, unsigned h);

void pg_model_split_tris(struct pg_model* model);
void pg_model_precalc_normals(struct pg_model* model);
void pg_model_precalc_tangents(struct pg_model* model);
void pg_model_precalc_verts(struct pg_model* model);
void pg_model_precalc_duplicates(struct pg_model* model, float tolerance);

/*  PRIMITIVES  */
void pg_model_quad(struct pg_model* model, vec2 tex_scale);
void pg_model_cube(struct pg_model* model, vec2 tex_scale);
void pg_model_cylinder(struct pg_model* model, int n, vec2 tex_scale);
void pg_model_cone(struct pg_model* model, int n, float base,
                   vec3 warp, vec2 tex_scale);
void pg_model_cone_trunc(struct pg_model* model, int n, float t, vec3 warp,
                         vec2 tex_scale, int tex_style);
void pg_model_icosphere(struct pg_model* model, int n);

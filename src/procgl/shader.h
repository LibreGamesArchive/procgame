struct pg_model;
struct pg_viewer;

enum pg_matrix {
    PG_MODEL_MATRIX,
    PG_NORMAL_MATRIX,
    PG_VIEW_MATRIX,
    PG_PROJECTION_MATRIX,
    PG_MODELVIEW_MATRIX,
    PG_PROJECTIONVIEW_MATRIX,
    PG_MVP_MATRIX
};

struct pg_shader {
    uint32_t components;
    GLuint vert, frag, prog;
    GLint component_idx[8];
    GLint mat_idx[7];
    mat4 matrix[7];
    /*  Per-shader interface    */
    void* data;
    void (*deinit)(void* data);
    void (*begin)(struct pg_shader* shader, struct pg_viewer* view);
};

/*  GLSL loading/compiling  */
int pg_compile_glsl(GLuint* vert, GLuint* frag, GLuint* prog,
                    const char* vert_filename, const char* frag_filename);
int pg_compile_glsl_static(GLuint* vert, GLuint* frag, GLuint* prog,
                           const char* vert_src, int vert_len,
                           const char* frag_src, int frag_len);
int pg_shader_load(struct pg_shader* shader,
                   const char* vert_filename, const char* frag_filename);
int pg_shader_load_static(struct pg_shader* shader,
                          const char* vert, int vert_len,
                          const char* frag, int frag_len);
/*  Matrix/component handling */
void pg_shader_link_matrix(struct pg_shader* shader, enum pg_matrix type,
                           const char* name);
void pg_shader_set_matrix(struct pg_shader* shader, enum pg_matrix type,
                          mat4 matrix);
void pg_shader_rebuild_matrices(struct pg_shader* shader);
void pg_shader_link_component(struct pg_shader* shader,
                              uint32_t comp, const char* name);
/*  Generates a VBO and VAO based on the shader and model components    */
void pg_shader_buffer_model(struct pg_shader* shader, struct pg_model* model);
/*  Check if a shader is currently active   */
int pg_shader_is_active(struct pg_shader* shader);

/*  Per-shader interface    */
void pg_shader_deinit(struct pg_shader* shader);
void pg_shader_begin(struct pg_shader* shader, struct pg_viewer* view);

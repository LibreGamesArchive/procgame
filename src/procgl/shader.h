
struct pg_model;
struct pg_viewer;

enum pg_shader_attribute {
    PG_ATTRIB_POSITION,
    PG_ATTRIB_NORMAL,
    PG_ATTRIB_TANGENT,
    PG_ATTRIB_BITANGENT,
    PG_ATTRIB_COLOR,
    PG_ATTRIB_TEXCOORD,
    PG_ATTRIB_HEIGHT,
    PG_NUM_ATTRIBS
};

static const struct pg_shader_attribute_info {
    char name[32];
    enum pg_data_type type;
    GLenum gltype;
    GLboolean normalized;
    GLint elements;
    int as_integer;
    int size;
} PG_SHADER_ATTRIBUTE_INFO[PG_NUM_ATTRIBS] = {
    [PG_ATTRIB_POSITION] = {
        .name = "v_position", .type = PG_VEC3,
        .gltype = GL_FLOAT, .elements = 3,
        .normalized = 0, .as_integer = 0,
        .size = sizeof(vec3), },
    [PG_ATTRIB_NORMAL] = {
        .name = "v_normal", .type = PG_VEC3,
        .gltype = GL_FLOAT, .elements = 3,
        .normalized = 0, .as_integer = 0,
        .size = sizeof(vec3), },
    [PG_ATTRIB_TANGENT] = {
        .name = "v_tangent", .type = PG_VEC3,
        .gltype = GL_FLOAT, .elements = 3,
        .normalized = 0, .as_integer = 0,
        .size = sizeof(vec3), },
    [PG_ATTRIB_BITANGENT] = {
        .name = "v_bitangent", .type = PG_VEC3,
        .gltype = GL_FLOAT, .elements = 3,
        .normalized = 0, .as_integer = 0,
        .size = sizeof(vec3), },
    [PG_ATTRIB_COLOR] = {
        .name = "v_color", .type = PG_UBVEC4,
        .gltype = GL_UNSIGNED_BYTE, .elements = 4,
        .normalized = 1, .as_integer = 0,
        .size = sizeof(ubvec4), },
    [PG_ATTRIB_TEXCOORD] = {
        .name = "v_tex_coord", .type = PG_VEC2,
        .gltype = GL_FLOAT, .elements = 2,
        .normalized = 0, .as_integer = 0,
        .size = sizeof(vec2), },
    [PG_ATTRIB_HEIGHT] = {
        .name = "v_height", .type = PG_FLOAT,
        .gltype = GL_FLOAT, .elements = 1,
        .normalized = 0, .as_integer = 0,
        .size = sizeof(float), },
};

struct pg_uniform {
    union {
        ivec4 i;
        uvec4 u;
        vec4 f;
        mat4 m;
        struct pg_texture* tex;
    };
};

struct pg_shader_uniform {
    enum pg_data_type type;
    struct pg_uniform data;
    int array_len;
    GLint idx;
};

typedef HTABLE_T(struct pg_shader_uniform) pg_shader_uniform_table_t;
typedef HTABLE_ENTRY pg_shader_uniform_t;

enum pg_matrix {
    PG_MODEL_MATRIX,
    PG_NORMAL_MATRIX,
    PG_VIEW_MATRIX,
    PG_PROJECTION_MATRIX,
    PG_MODELVIEW_MATRIX,
    PG_PROJECTIONVIEW_MATRIX,
    PG_MVP_MATRIX,
    PG_COMMON_MATRICES,
};

struct pg_shader {
    /*  Handles for the shader program  */
    GLuint vert, frag, prog;
    /*  Set if shader has static vertices; should not use a model   */
    int static_verts;
    /*  Set if the shader should only draw once with a full-screen quad */
    int is_postproc;
    /*  Uniforms: matrices, textures, and a generic string->index table */
    pg_shader_uniform_t matrix[PG_COMMON_MATRICES];
    pg_shader_uniform_t fbdepth;
    pg_shader_uniform_t fbtex[8];
    pg_shader_uniform_t tex[8];
    pg_shader_uniform_t tex_rect[8];
    pg_shader_uniform_table_t uniforms;
};

typedef HTABLE_ENTRY pg_shader_ref_t;
typedef HTABLE_T(struct pg_shader) pg_shader_table_t;

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

/*  Generates a VBO and VAO based on the shader and model components    */
#define pg_shader_buffer_model(shader, model) \
    pg_shader_buffer_model_(shader, model, __FILE__, __LINE__)
void pg_shader_buffer_model_(struct pg_shader* shader, struct pg_model* model,
                             const char* file, int line);
/*  Check if a shader is currently active   */
int pg_shader_is_active(struct pg_shader* shader);

/*  Per-shader interface    */
void pg_shader_deinit(struct pg_shader* shader);
void pg_shader_begin(struct pg_shader* shader);

void pg_shader_set_matrix(struct pg_shader* shader,
                          enum pg_matrix dst, mat4 mat);
void pg_shader_set_texture(struct pg_shader* shader,
                           int idx, struct pg_texture* tex);
void pg_shader_set_tex_rect(struct pg_shader* shader,
                            int idx, vec4 tex_rect);
pg_shader_uniform_t pg_shader_get_uniform(struct pg_shader* shader, char* name);
void pg_shader_uniform_by_name(struct pg_shader* shader,
                               char* name, struct pg_uniform* uni);
void pg_shader_uniforms_from_table(struct pg_shader* shader,
                                   pg_shader_uniform_table_t* table);
void pg_shader_uniform(struct pg_shader* shader, pg_shader_uniform_t sh_uni,
                       struct pg_uniform* uni);

#if 0
/*  Matrix/component handling */
void pg_shader_rebuild_matrices(struct pg_shader* shader);

#endif

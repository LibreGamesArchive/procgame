enum pg_model_component {
    PG_MODEL_COMPONENT_POSITION =   (1 << 0),
    PG_MODEL_COMPONENT_COLOR =      (1 << 1),
    PG_MODEL_COMPONENT_UV =         (1 << 2),
    PG_MODEL_COMPONENT_NORMAL =     (1 << 3),
    PG_MODEL_COMPONENT_TANGENT =    (1 << 4),
    PG_MODEL_COMPONENT_BITANGENT =  (1 << 5),
    PG_MODEL_COMPONENT_HEIGHT =     (1 << 6),
    PG_MODEL_COMPONENT_TAN_BITAN =
        (PG_MODEL_COMPONENT_TANGENT | PG_MODEL_COMPONENT_BITANGENT)
};

enum pg_model_component_i {
    PG_MODEL_COMPONENT_POSITION_I,
    PG_MODEL_COMPONENT_COLOR_I,
    PG_MODEL_COMPONENT_UV_I,
    PG_MODEL_COMPONENT_NORMAL_I,
    PG_MODEL_COMPONENT_TANGENT_I,
    PG_MODEL_COMPONENT_BITANGENT_I,
    PG_MODEL_COMPONENT_HEIGHT_I,
};

/*  This is meant to be used for exchanging verts between models, and also
    adding raw verts to models  */
struct pg_vertex_full {
    uint32_t components;
    vec3 pos;
    vec3 color;
    vec2 uv;
    vec3 normal;
    vec3 tangent, bitangent;
    float height;
};

struct pg_model_buffer {
    struct pg_shader* shader;
    GLuint vbo;
    GLuint vao;
    int dirty_buffers;
};

typedef struct { vec4 v; } vec4_t;
typedef struct { vec3 v; } vec3_t;
typedef struct { vec2 v; } vec2_t;

struct pg_model {
    uint32_t components;
    unsigned v_count;
    ARR_T(vec3_t) pos;
    ARR_T(vec4_t) color;
    ARR_T(vec2_t) uv;
    ARR_T(vec3_t) normal; 
    ARR_T(vec3_t) tangent;
    ARR_T(vec3_t) bitangent;
    ARR_T(float) height;
    int active;
    ARR_T(struct pg_model_buffer) buffers;
    int dirty_tris;
    GLuint ebo;
    ARR_T(unsigned) tris;
};

/*  Setup+cleanup   */
void pg_model_init(struct pg_model* model);
void pg_model_reset(struct pg_model* model);
void pg_model_deinit(struct pg_model* model);
/*  Shader handling */
void pg_model_buffer(struct pg_model* model);
void pg_model_begin(struct pg_model* model, struct pg_shader* shader);
void pg_model_draw(struct pg_model* model, mat4 transform);
/*  Raw vertex/triangle building    */
void pg_model_reserve_verts(struct pg_model* model, unsigned count);
void pg_model_reserve_tris(struct pg_model* model, unsigned count);
void pg_model_set_vertex(struct pg_model* model, struct pg_vertex_full* v,
                         unsigned i);
void pg_model_get_vertex(struct pg_model* model, struct pg_vertex_full* out,
                         unsigned i);
unsigned pg_model_add_vertex(struct pg_model* model,
                             struct pg_vertex_full* vert);
void pg_model_add_triangle(struct pg_model* model, unsigned v0,
                           unsigned v1, unsigned v2);
/*  Composition/transformation  */
void pg_model_append(struct pg_model* dst, struct pg_model* src,
                         mat4 transform);
void pg_model_transform(struct pg_model* model, mat4 transform);
/*  Component generation    */
void pg_model_precalc_normals(struct pg_model* model);
void pg_model_precalc_ntb(struct pg_model* model);
/*  Duplicate vertex handling   */
void pg_model_split_tris(struct pg_model* model);
void pg_model_blend_duplicates(struct pg_model* model, float tolerance);
void pg_model_join_duplicates(struct pg_model* model,
                              float pos, float color, float uv,
                              float normal, float height);

/*  PRIMITIVES  model_prims.c   */
void pg_model_quad(struct pg_model* model, vec2 tex_scale);
void pg_model_cube(struct pg_model* model, vec2 tex_scale);
void pg_model_cylinder(struct pg_model* model, int n, vec2 tex_scale);
void pg_model_cone(struct pg_model* model, int n, float base,
                   vec3 warp, vec2 tex_scale);
void pg_model_cone_trunc(struct pg_model* model, int n, float t, vec3 warp,
                         vec2 tex_scale, int tex_style);
void pg_model_icosphere(struct pg_model* model, int n);

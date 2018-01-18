struct pg_model_attribute_array {
    enum pg_data_type type;
    int used;
    union {
        ARR_T(int8_t) b1;
        ARR_T(bvec2_t) b2;
        ARR_T(bvec3_t) b3;
        ARR_T(bvec4_t) b4;
        ARR_T(uint8_t) ub1;
        ARR_T(ubvec2_t) ub2;
        ARR_T(ubvec3_t) ub3;
        ARR_T(ubvec4_t) ub4;
        ARR_T(int32_t) i1;
        ARR_T(ivec2_t) i2;
        ARR_T(ivec3_t) i3;
        ARR_T(ivec4_t) i4;
        ARR_T(uint32_t) u1;
        ARR_T(uvec2_t) u2;
        ARR_T(uvec3_t) u3;
        ARR_T(uvec4_t) u4;
        ARR_T(float) f1;
        ARR_T(vec2_t) f2;
        ARR_T(vec3_t) f3;
        ARR_T(vec4_t) f4;
    };
};

struct pg_model_attribute {
    union {
        bvec4 b;
        ubvec4 ub;
        ivec4 i;
        uvec4 u;
        vec4 f;
    };
};

struct pg_model_buffer {
    void* buffer;
    GLuint ebo;
    GLuint vbo;
    GLuint vao;
};

struct pg_tri { unsigned t[3]; };

struct pg_model {
    unsigned v_count;
    struct pg_model_attribute_array attribs[PG_NUM_ATTRIBS];
    ARR_T(struct pg_tri) tris;
    struct pg_model_buffer buf;
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
void pg_model_reserve_attrib(struct pg_model* model,
                             enum pg_shader_attribute attr, unsigned count);
void pg_model_reserve_verts(struct pg_model* model, unsigned count);
void pg_model_set_attrib(struct pg_model* model,
                         enum pg_shader_attribute attr, unsigned idx,
                         struct pg_model_attribute* set);
void pg_model_add_triangle(struct pg_model* model, unsigned v0,
                           unsigned v1, unsigned v2);
/*  Composition/transformation  */
void pg_model_append(struct pg_model* dst, struct pg_model* src,
                     mat4 transform);
void pg_model_transform(struct pg_model* model, mat4 transform);
void pg_model_invert_tris(struct pg_model* model);
/*  Component generation    */
void pg_model_precalc_normals(struct pg_model* model);
void pg_model_precalc_ntb(struct pg_model* model);
/*  Vertex duplication/de-duplication/duplicate handling    */
void pg_model_seams_tris(struct pg_model* model);
void pg_model_seams_cardinal_directions(struct pg_model* model);
/*  Duplicate vertex handling   */
void pg_model_split_tris(struct pg_model* model);
void pg_model_blend_duplicates(struct pg_model* model, float tolerance);
void pg_model_join_duplicates(struct pg_model* model, float t);
void pg_model_warp_verts(struct pg_model* model);
/*  Generic triangle operations */
void pg_model_get_face_normal(struct pg_model* model, unsigned tri, vec3 norm);


/*  PRIMITIVES  model_prims.c   */
void pg_model_quad(struct pg_model* model, vec2 tex_scale);
void pg_model_cube(struct pg_model* model, vec2 tex_scale);
void pg_model_rect_prism(struct pg_model* model, vec3 scale, vec4* face_uv);
void pg_model_cylinder(struct pg_model* model, int n, vec2 tex_scale);
void pg_model_cone(struct pg_model* model, int n, float base,
                   vec3 warp, vec2 tex_scale);
void pg_model_cone_trunc(struct pg_model* model, int n, float t, vec3 warp,
                         vec2 tex_scale, int tex_style);
void pg_model_icosphere(struct pg_model* model, int n);

/*  This stuff is disabled for now due to pg_model overhaul     */
#if 0
void pg_model_precalc_uv(struct pg_model* model);
void pg_model_face_projection_overlap(struct pg_model* model, vec3 proj,
                                      unsigned tri0, unsigned tri1);

/*  Collision functions - Return the nearest colliding triangle index, and set
    'out' argument to the push vector to get out of the collision   */
int pg_model_collide_sphere(struct pg_model* model, vec3 out, vec3 const pos,
                            float r, int n);
int pg_model_collide_sphere_sub(struct pg_model* model, vec3 out, vec3 const pos,
                                float r, int n, unsigned sub_i, unsigned sub_len);
int pg_model_collide_ellipsoid(struct pg_model* model, vec3 out, vec3 const pos,
                               vec3 const r, int n);
int pg_model_collide_ellipsoid_sub(struct pg_model* model, vec3 out, vec3 const pos,
                                   vec3 const r, int n, unsigned sub_i, unsigned sub_len);

/*  Generate a model from an SDF tree   marching_cubes.c    */
struct pg_sdf_tree;
void pg_model_sdf(struct pg_model* model, struct pg_sdf_tree* sdf, float p);
#endif

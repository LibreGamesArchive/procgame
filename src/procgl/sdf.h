#include <stdint.h>

enum pg_sdf_type {
    /*  Primitives  */
    PG_SDF_NODE_PLANE,
    PG_SDF_NODE_BOX,
    PG_SDF_NODE_ELLIPSOID,
    PG_SDF_NODE_SPHERE,
    PG_SDF_NODE_CYLINDER,
    PG_SDF_NODE_CONE,
    PG_SDF_NODE_CAPSULE,
    PG_SDF_NODE_TORUS,
    PG_SDF_NODE_WAVE,
    PG_SDF_NODE_PRIMITIVES__,
    /*  Math structures */
    PG_SDF_NODE_MATRIX,
    PG_SDF_NODE_VECTOR,
    PG_SDF_NODE_SCALAR,
    PG_SDF_NODE_STRUCTURES__,
    /*  Transformations */
    PG_SDF_NODE_TRANSFORM_MATRIX,
    PG_SDF_NODE_ROTATE,
    PG_SDF_NODE_TRANSLATE,
    PG_SDF_NODE_SCALE,
    PG_SDF_NODE_WARP,
    PG_SDF_NODE_THRESHOLD,
    PG_SDF_NODE_TRANSFORMS__,
    /*  Boolean operations  */
    PG_SDF_NODE_UNION,
    PG_SDF_NODE_BLEND,
    PG_SDF_NODE_SUBTRACT,
    PG_SDF_NODE_INTERSECT,
    PG_SDF_NODE_BOOLEANS__,
    /*  Special nodes   */
    PG_SDF_NODE_ARGUMENT,
    PG_SDF_NODE_SUBTREE,
    PG_SDF_NODE_SPECIALS__,
    PG_SDF_NODE_NULL
};

struct pg_sdf_node {
    enum pg_sdf_type type;
    union {
        /*  Primitive definitions   */
        vec4 plane;
        vec3 box, ellipsoid;
        vec2 cylinder, cone, capsule, torus;
        float sphere;
        struct pg_wave* wave;
        /*  Math structures */
        mat4 matrix;
        vec3 vector;
        float scalar;
        /*  Domain transformation function pointer  */
        void (*warp)(vec3, vec3);
        /*  Index of argument in arg list   */
        int arg;
        /*  Pointer to another tree to sample   */
        struct pg_sdf_tree* subtree;
        /*  Operation arguments */
        float blend;
    };
};

struct pg_sdf_tree {
    ARR_T(struct pg_sdf_node) op_tree;
    ARR_T(struct pg_sdf_node) args;
};

void pg_sdf_tree_init(struct pg_sdf_tree* sdf);
void pg_sdf_tree_reset(struct pg_sdf_tree* sdf);
void pg_sdf_tree_deinit(struct pg_sdf_tree* sdf);
float pg_sdf_tree_sample(struct pg_sdf_tree* sdf, vec3 s);
struct pg_sdf_node* pg_sdf_tree_add_arg(struct pg_sdf_tree* sdf, int arg);
struct pg_sdf_node* pg_sdf_tree_get_arg(const struct pg_sdf_tree* sdf, int arg);
struct pg_sdf_node* pg_sdf_node_child(const struct pg_sdf_tree* sdf,
                                      const struct pg_sdf_node* node, int side);
void pg_sdf_node_sample(const struct pg_sdf_tree* tree,
                        struct pg_sdf_node* node,
                        vec3 p, vec4 out);  /*  sdf_functions.c */

/*  Parsing text SDF definitions; this structure is used in sdf_keywords.gperf,
    which generates sdf_keywords.gperf.c, which is a compile-time calculated
    perfect hash function, for matching keywords in the text.   */
void pg_sdf_tree_parse(struct pg_sdf_tree* sdf, const char* src, unsigned len);
int pg_sdf_tree_print(struct pg_sdf_tree* sdf, char* out, unsigned max);

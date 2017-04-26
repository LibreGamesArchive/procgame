#include "ext/linmath.h"
#include "arr.h"
#include "wave.h"
#include "sdf.h"

static const vec3 vec3_zero = { 0, 0, 0 };

#define SDF_DEFINE_FUNC(f) \
    void f(const struct pg_sdf_tree* tree, struct pg_sdf_node* node, \
           vec3 p, vec4 out)

/*  ----------------------------------------------
    PRIMITIVE DISTANCE FUNCTIONS
    ----------------------------------------------  */

SDF_DEFINE_FUNC(func_plane)
{
    vec4_set(out, p[0], p[1], p[2],
        vec3_mul_inner(p, node->plane) + node->plane[3]);
}

SDF_DEFINE_FUNC(func_box)
{
    vec3 d;
    vec3_abs(d, p);
    vec3_sub(d, d, node->box);
    vec3 p_max, p_min;
    vec3_max(p_max, d, vec3_zero);
    vec3_min(p_min, d, vec3_zero);
    vec4_set(out, d[0], d[1], d[2], 
        vec3_len(p_max) + vec3_vmax(p_min));
}

SDF_DEFINE_FUNC(func_ellipsoid)
{
    vec3 pr = { p[0] / node->ellipsoid[0],
                p[1] / node->ellipsoid[1],
                p[2] / node->ellipsoid[2] };
    /*  The point in node-space here represents the ellipsoid as a sphere   */
    vec4_set(out, pr[0], pr[1], pr[2],
        (vec3_len(pr) - 1.0) * vec3_vmin(node->ellipsoid));
}

SDF_DEFINE_FUNC(func_sphere)
{
    vec4_set(out, p[0], p[1], p[2], vec3_len(p) - (node->sphere));
}

SDF_DEFINE_FUNC(func_cylinder)
{
    float d = vec2_len(p) - node->cylinder[0];
    vec4_set(out, p[0], p[1], p[2],
        MAX(d, fabsf(p[2]) - node->cylinder[1]));
}

SDF_DEFINE_FUNC(func_cone)
{
    vec2 q = { vec2_len(p), p[2] };
    vec2 tip = { q[0], q[1] - node->cone[1] };
    vec2 m = { node->cone[1], node->cone[0] };
    vec2 mn;
    vec2_normalize(mn, m);
    float md = vec2_mul_inner(tip, mn);
    float d = MAX(md, -q[1]);
    vec2 m90 = { mn[1], -mn[0] };
    float projected = vec2_mul_inner(tip, m90);
    if((q[1] > node->cone[1]) && (projected < 0)) {
        d = MAX(d, vec2_len(tip));
    }
    if((q[0] > node->cone[0]) && (projected > vec2_len(m))) {
        vec2 qsubr = { q[0] - node->cone[0], q[1] };
        d = MAX(d, vec2_len(qsubr));
    }
    vec4_set(out,
             p[0] / node->cone[0], p[1] / node->cone[0], p[2] / node->cone[1],
             d);
}

SDF_DEFINE_FUNC(func_capsule)
{
    vec3 cap = { p[0], fabsf(p[2]) - node->capsule[1], p[1] };
    vec4_set(out, p[0] / node->capsule[0],
                  p[1] / node->capsule[0],
                  p[2] / node->capsule[1],
        fabsf(p[2]) > node->capsule[1]
            ? vec2_len(p) - node->capsule[0]
            : vec3_len(cap) - node->capsule[0]);
}

SDF_DEFINE_FUNC(func_torus)
{
    vec2 lr = { vec2_len(p) - node->torus[0], p[2] };
    vec4_set(out, p[0] / (node->torus[0] + node->torus[1]),
                  p[1] / (node->torus[0] + node->torus[1]),
                  p[2] / (node->torus[1]),
        vec2_len(lr) - node->torus[1]);
}

SDF_DEFINE_FUNC(func_wave)
{
    vec4_set(out, p[0], p[1], p[2], pg_wave_sample(node->wave, 3, p));
}

/*  ----------------------------------------------
    MATH STRUCTURES
    ----------------------------------------------  */
SDF_DEFINE_FUNC(func_matrix)
{
    vec4 p4 = { p[0], p[1], p[2], 1 };
    vec4 new_p4;
    mat4_mul_vec4(new_p4, node->matrix, new_p4);
    vec3_sub(p4, new_p4, p);
    vec4_set(out, new_p4[0], new_p4[1], new_p4[2], vec3_len(p4));
}


SDF_DEFINE_FUNC(func_vector)
{
    vec3 d;
    vec3_add(d, p, node->vector);
    vec4_set(out, node->vector[0], node->vector[1], node->vector[2],
        vec3_len(d) - 1.0);
}

SDF_DEFINE_FUNC(func_scalar)
{
    vec4_set(out, p[0], p[1], p[2], node->scalar);
}

/*  ----------------------------------------------
    TRANSFORM NODES
    ----------------------------------------------  */
SDF_DEFINE_FUNC(func_transform)
{
    vec4 in;
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in);
    pg_sdf_node_sample(tree, child[1], in, out);
}

SDF_DEFINE_FUNC(func_rotate)
{
    vec4 in;
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in);
    mat4 rotate;
    mat4_identity(rotate);
    mat4_rotate_Y(rotate, rotate, in[2]);
    mat4_rotate_X(rotate, rotate, in[1]);
    mat4_rotate_Z(rotate, rotate, in[0]);
    mat4_mul_vec4(in, rotate, in);
    pg_sdf_node_sample(tree, child[1], in, out);
}

SDF_DEFINE_FUNC(func_translate)
{
    vec4 in;
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in);
    vec3_add(in, in, p);
    pg_sdf_node_sample(tree, child[1], in, out);
}

SDF_DEFINE_FUNC(func_scale)
{
    vec4 in;
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in);
    vec3_mul(in, in, p);
    pg_sdf_node_sample(tree, child[1], in, out);
}

SDF_DEFINE_FUNC(func_warp)
{
    vec3 w;
    node->warp(w, p);
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[1], w, out);
}

SDF_DEFINE_FUNC(func_threshold)
{
    vec4 in;
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in);
    pg_sdf_node_sample(tree, child[1], p, out);
    out[3] += in[3];
}

/*  ----------------------------------------------
    BOOLEAN OPERATIONS
    ----------------------------------------------  */
SDF_DEFINE_FUNC(func_union)
{
    vec4 in[2];
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in[0]);
    pg_sdf_node_sample(tree, child[1], p, in[1]);
    float d = MIN(in[0][3], in[1][3]);
    vec4_set(out, p[0], p[1], p[2], d);
}

SDF_DEFINE_FUNC(func_blend)
{
    vec4 in[2];
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in[0]);
    pg_sdf_node_sample(tree, child[1], p, in[1]);
    float d = smin(in[0][3], in[1][3], node->blend);
    vec4_set(out, p[0], p[1], p[2], d);
}

SDF_DEFINE_FUNC(func_subtract)
{
    vec4 in[2];
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in[0]);
    pg_sdf_node_sample(tree, child[1], p, in[1]);
    float d = MAX(in[0][3], -in[1][3]);
    vec4_set(out, p[0], p[1], p[2], d);
}

SDF_DEFINE_FUNC(func_intersect)
{
    vec4 in[2];
    struct pg_sdf_node* child[2] = {
        pg_sdf_node_child(tree, node, 0),
        pg_sdf_node_child(tree, node, 1) };
    pg_sdf_node_sample(tree, child[0], p, in[0]);
    pg_sdf_node_sample(tree, child[1], p, in[1]);
    float d = MAX(in[0][3], in[1][3]);
    vec4_set(out, p[0], p[1], p[2], d);
}

/*  ----------------------------------------------
    SPECIAL NODES
    ----------------------------------------------  */
SDF_DEFINE_FUNC(func_argument)
{
    struct pg_sdf_node* arg = pg_sdf_tree_get_arg(tree, node->arg);
    if(!arg) {
        vec4_set(out, p[0], p[1], p[2], vec3_len(p) - 1);
        return;
    }
    int arg_i = node->arg;
    *node = *arg;
    pg_sdf_node_sample(tree, node, p, out);
    *node = (struct pg_sdf_node){ PG_SDF_NODE_ARGUMENT, .arg = arg_i };
}

SDF_DEFINE_FUNC(func_subtree)
{
    vec4_set(out, p[0], p[1], p[2], pg_sdf_tree_sample(node->subtree, p));
}

/*  Function pointer array  */
static void (*sample_func[PG_SDF_NODE_NULL])(const struct pg_sdf_tree* tree,
                                              struct pg_sdf_node* node,
                                              vec3 p, vec4 out) = {
    [PG_SDF_NODE_PLANE] = func_plane,
    [PG_SDF_NODE_BOX] = func_box,
    [PG_SDF_NODE_ELLIPSOID] = func_ellipsoid,
    [PG_SDF_NODE_SPHERE] = func_sphere,
    [PG_SDF_NODE_CYLINDER] = func_cylinder,
    [PG_SDF_NODE_CONE] = func_cone,
    [PG_SDF_NODE_CAPSULE] = func_capsule,
    [PG_SDF_NODE_TORUS] = func_torus,
    [PG_SDF_NODE_WAVE] = func_wave,
    [PG_SDF_NODE_MATRIX] = func_matrix,
    [PG_SDF_NODE_VECTOR] = func_vector,
    [PG_SDF_NODE_SCALAR] = func_scalar,
    [PG_SDF_NODE_TRANSFORM_MATRIX] = func_transform,
    [PG_SDF_NODE_ROTATE] = func_rotate,
    [PG_SDF_NODE_TRANSLATE] = func_translate,
    [PG_SDF_NODE_SCALE] = func_scale,
    [PG_SDF_NODE_WARP] = func_warp,
    [PG_SDF_NODE_THRESHOLD] = func_threshold,
    [PG_SDF_NODE_UNION] = func_union,
    [PG_SDF_NODE_BLEND] = func_blend,
    [PG_SDF_NODE_SUBTRACT] = func_subtract,
    [PG_SDF_NODE_INTERSECT] = func_intersect,
    [PG_SDF_NODE_ARGUMENT] = func_argument,
    [PG_SDF_NODE_SUBTREE] = func_subtree
};

/*  Public interface    */
void pg_sdf_node_sample(const struct pg_sdf_tree* tree,
                        struct pg_sdf_node* node,
                        vec3 p, vec4 out)
{
    if(!sample_func[node->type]) {
        vec4_set(out, 0, 0, 0, vec3_len(p) - 1);
        return;
    }
    if(sample_func[node->type]) sample_func[node->type](tree, node, p, out);
}

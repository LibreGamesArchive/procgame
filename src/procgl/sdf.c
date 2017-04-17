#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "ext/linmath.h"
#include "arr.h"
#include "wave.h"
#include "sdf.h"

struct pg_sdf_keyword {
    const char* name;
    enum pg_sdf_type type;
};
struct pg_sdf_keyword* pg_sdf_keyword_read(register const char* str,
                                           register unsigned int len);
#include "sdf_keywords.gperf.c"

/*  Create and/or get an argument node  */
struct pg_sdf_node* pg_sdf_tree_get_arg(const struct pg_sdf_tree* sdf, int arg)
{
    if(arg >= sdf->args.cap
    || sdf->args.data[arg].type == PG_SDF_NODE_NULL) return NULL;
    return &sdf->args.data[arg];
}

struct pg_sdf_node* pg_sdf_tree_add_arg(struct pg_sdf_tree* sdf, int arg)
{
    if(arg >= sdf->args.cap) {
        ARR_RESERVE(sdf->args, arg + 1);
        sdf->args.len = arg + 1;
    }
    return sdf->args.data + arg;
}

/*  Add a child node    */
static int tree_add_child(struct pg_sdf_tree* sdf, int node_i, int side)
{
    if(!sdf) return 0;
    int child = 2 * node_i + 1 + (!!side);
    if(child >= sdf->op_tree.cap) {
        ARR_RESERVE(sdf->op_tree, child + 1);
        sdf->op_tree.len = child + 1;
    }
    return child;
}

/*  Public interface    */
void pg_sdf_tree_init(struct pg_sdf_tree* sdf)
{
    ARR_INIT(sdf->op_tree);
    ARR_INIT(sdf->args);
}

void pg_sdf_tree_reset(struct pg_sdf_tree* sdf)
{
    ARR_TRUNCATE(sdf->op_tree, 0);
    ARR_TRUNCATE(sdf->args, 0);
}

void pg_sdf_tree_deinit(struct pg_sdf_tree* sdf)
{
    ARR_DEINIT(sdf->op_tree);
    ARR_DEINIT(sdf->args);
}

float pg_sdf_tree_sample(struct pg_sdf_tree* sdf, vec3 s)
{
    struct pg_sdf_node* node = &sdf->op_tree.data[0];
    vec4 out;
    pg_sdf_node_sample(sdf, node, s, out);
    return out[3];
}

struct pg_sdf_node* pg_sdf_node_child(const struct pg_sdf_tree* sdf,
                                      const struct pg_sdf_node* node, int side)
{
    int idx = node - sdf->op_tree.data;
    int child = 2 * idx + 1 + (!!side);
    if(child >= sdf->op_tree.cap) return NULL;
    return &sdf->op_tree.data[child];
}

/*  Parsing stuff   */
#define EAT_SPACES while(src_i < end && *src_i && isspace(*src_i)) ++src_i;
#define READ_WORD do { \
        EAT_SPACES; \
        for(i = 0; i < 32 && src_i < end && *src_i \
                   && (isalnum(*src_i) || *src_i == '.') ; ++i) \
            tmp[i] = *(src_i++); \
        tmp[i] = '\0'; \
        if(src_i >= end) return end; \
    } while(0)
#define READ_FLOAT(v) do { \
        READ_WORD; \
        if(src_i >= end) return end; \
        char* tmp_end; \
        (v) = strtof(tmp, &tmp_end); \
        if(src_i >= end) { \
            return src_i; \
        } else if(tmp_end - tmp != i) { \
            printf("procgl SDF parse error: bad constant: %s\n", tmp); \
            return src_i; \
        } \
    } while(0)
#define READ_ARG do { \
        char* tmp_end; \
        int arg = strtol(tmp + 1, &tmp_end, 10); \
        if(tmp_end - tmp <= i) { \
            printf("procgl SDF parse error: bad argument id: %s\n", tmp); \
            return src_i; \
        } \
        *node = (struct pg_sdf_node) { \
            .type = PG_SDF_NODE_ARGUMENT, \
            .arg = arg }; \
        return src_i; \
    } while(0)
#define RECURSE do { \
        int child[2]  = { tree_add_child(sdf, node_i, 0), \
                          tree_add_child(sdf, node_i, 1) }; \
        src_i = parse_node_recursive(sdf, child[0], src_i, end); \
        src_i = parse_node_recursive(sdf, child[1], src_i, end); \
        return src_i; \
    } while(0)

static const char* parse_node_recursive(struct pg_sdf_tree* sdf, int node_i,
                                        const char* src, const char* end)
{
    int i = 0;
    char tmp[32] = {};
    const char* src_i = src;
    struct pg_sdf_node* node = &sdf->op_tree.data[node_i];
    EAT_SPACES;
    if(*src_i == '(') {
        ++src_i;
        READ_WORD;
        if(tmp[0] == '$') {
            READ_ARG;
        }
        struct pg_sdf_keyword* kw = pg_sdf_keyword_read(tmp, i);
        if(!kw) {
            printf("procgl SDF parse error: unrecognized keyword: %s\n", tmp);
            return src_i;
        }
        *node = (struct pg_sdf_node){ .type = kw->type };
        if(kw->type == PG_SDF_NODE_VECTOR) {
            READ_FLOAT(node->vector[0]);
            READ_FLOAT(node->vector[1]);
            READ_FLOAT(node->vector[2]);
            return src_i;
        } else if(kw->type == PG_SDF_NODE_MATRIX) {
            int j, k;
            for(j = 0; j < 4; ++j) for(k = 0; k < 4; ++j) {
                READ_FLOAT(node->matrix[j][k]);
            }
            return src_i;
        } else if(kw->type < PG_SDF_NODE_PRIMITIVES__) {
            switch(kw->type) {
            case PG_SDF_NODE_PLANE:
                READ_FLOAT(node->plane[0]);
                READ_FLOAT(node->plane[1]);
                READ_FLOAT(node->plane[2]);
                READ_FLOAT(node->plane[3]);
                break;
            case PG_SDF_NODE_BOX:
                READ_FLOAT(node->box[0]);
                READ_FLOAT(node->box[1]);
                READ_FLOAT(node->box[2]);
                break;
            case PG_SDF_NODE_ELLIPSOID:
                READ_FLOAT(node->ellipsoid[0]);
                READ_FLOAT(node->ellipsoid[1]);
                READ_FLOAT(node->ellipsoid[2]);
                break;
            case PG_SDF_NODE_SPHERE:
                READ_FLOAT(node->sphere);
                break;
            case PG_SDF_NODE_CYLINDER:
                READ_FLOAT(node->cylinder[0]);
                READ_FLOAT(node->cylinder[1]);
                break;
            case PG_SDF_NODE_CONE:
                READ_FLOAT(node->cone[0]);
                READ_FLOAT(node->cone[1]);
                break;
            case PG_SDF_NODE_CAPSULE:
                READ_FLOAT(node->capsule[0]);
                READ_FLOAT(node->capsule[1]);
                break;
            case PG_SDF_NODE_TORUS:
                READ_FLOAT(node->torus[0]);
                READ_FLOAT(node->torus[1]);
                break;
            case PG_SDF_NODE_WAVE: default:
                break;
            }
            EAT_SPACES;
            if(*src_i != ')') {
                printf("procgl SDF parse error: unterminated node definition\n");
            }
            return ++src_i;
        } else if(kw->type < PG_SDF_NODE_BOOLEANS__) {
            RECURSE;
            return src_i;
        } else if(kw->type < PG_SDF_NODE_TRANSFORMS__) {
            if(kw->type == PG_SDF_NODE_WARP) {
                EAT_SPACES;
                READ_WORD;
                /*  Get warp function name  */
            } else if(kw->type == PG_SDF_NODE_THRESHOLD) {
                READ_FLOAT(node->blend);
            }
            RECURSE;
            return src_i;
        }
        EAT_SPACES;
        if(*src_i != ')') {
            printf("procgl SDF parse error: unterminated node definition\n");
        }
        return ++src_i;
    } else if(*src_i == '$') {
        READ_ARG;
    } else {
        printf("procgl SDF parse error: unexpected character: %c\n", *src_i);
        return src_i;
    }
    return src_i;
}

#undef EAT_SPACES
#undef READ_WORD
#undef READ_FLOAT
#undef READ_ARG
#undef RECURSE

void pg_sdf_tree_parse(struct pg_sdf_tree* sdf, const char* src, unsigned len)
{
    pg_sdf_tree_reset(sdf);
    struct pg_sdf_node root = { PG_SDF_NODE_NULL };
    ARR_PUSH(sdf->op_tree, root);
    const char* end = src + len;
    const char* src_i = parse_node_recursive(sdf, 0, src, end);
}

static const char* printouts[] = {
    [PG_SDF_NODE_PLANE] = "PLANE",
    [PG_SDF_NODE_BOX] = "BOX",
    [PG_SDF_NODE_ELLIPSOID] = "ELLIPSOID",
    [PG_SDF_NODE_SPHERE] = "SPHERE",
    [PG_SDF_NODE_CYLINDER] = "CYLINDER",
    [PG_SDF_NODE_CONE] = "CONE",
    [PG_SDF_NODE_CAPSULE] = "CAPSULE",
    [PG_SDF_NODE_TORUS] = "TORUS",
    [PG_SDF_NODE_WAVE] = "WAVE",
    [PG_SDF_NODE_PRIMITIVES__] = "",
    [PG_SDF_NODE_MATRIX] = "matrix",
    [PG_SDF_NODE_VECTOR] = "vector",
    [PG_SDF_NODE_STRUCTURES__] = "",
    [PG_SDF_NODE_TRANSFORM_MATRIX] = "transform",
    [PG_SDF_NODE_ROTATE] = "rotate",
    [PG_SDF_NODE_TRANSLATE] = "translate",
    [PG_SDF_NODE_SCALE] = "scale",
    [PG_SDF_NODE_WARP] = "warp",
    [PG_SDF_NODE_THRESHOLD] = "threshold",
    [PG_SDF_NODE_TRANSFORMS__] = "",
    [PG_SDF_NODE_UNION] = "union",
    [PG_SDF_NODE_BLEND] = "blend",
    [PG_SDF_NODE_SUBTRACT] = "subtract",
    [PG_SDF_NODE_INTERSECT] = "intersect",
    [PG_SDF_NODE_BOOLEANS__] = "",
    [PG_SDF_NODE_ARGUMENT] = "argument",
    [PG_SDF_NODE_SUBTREE] = "subtree",
    [PG_SDF_NODE_NULL] = "NULL" };

static char* print_node_recursive(const struct pg_sdf_tree* sdf,
                                const struct pg_sdf_node* node,
                                char* out, const char* end)
{
    if(out >= end) return out;
    char* out_i = out;
    if(node->type < PG_SDF_NODE_STRUCTURES__) {
        switch(node->type) {
        case PG_SDF_NODE_PLANE:
            out_i += snprintf(out, end - out, "(%s dir={%f %f %f} t=%f) ",
                printouts[node->type],
                node->plane[0], node->plane[1], node->plane[2], node->plane[3]);
            break;
        case PG_SDF_NODE_BOX:
            out_i += snprintf(out, end - out, "(%s w=%f l=%f h=%f) ",
                printouts[node->type],
                node->box[0], node->box[1], node->box[2]);
            break;
        case PG_SDF_NODE_ELLIPSOID:
            out_i += snprintf(out, end - out, "(%s w=%f l=%f h=%f) ",
                printouts[node->type],
                node->ellipsoid[0], node->ellipsoid[1], node->ellipsoid[2]);
            break;
        case PG_SDF_NODE_SPHERE:
            out_i += snprintf(out, end - out, "(%s r=%f) ",
                printouts[node->type], node->sphere);
            break;
        case PG_SDF_NODE_CYLINDER:
            out_i += snprintf(out, end - out, "(%s r=%f h=%f) ",
                printouts[node->type], node->cylinder[0], node->cylinder[1]);
            break;
        case PG_SDF_NODE_CONE:
            out_i += snprintf(out, end - out, "(%s r=%f h=%f) ",
                printouts[node->type], node->cone[0], node->cone[1]);
            break;
        case PG_SDF_NODE_CAPSULE:
            out_i += snprintf(out, end - out, "(%s r=%f h=%f) ",
                printouts[node->type], node->capsule[0], node->capsule[1]);
            break;
        case PG_SDF_NODE_TORUS:
            out_i += snprintf(out, end - out, "(%s l=%f s=%f) ",
                printouts[node->type], node->torus[0], node->torus[1]);
            break;
        case PG_SDF_NODE_WAVE:
            out_i += snprintf(out, end - out, "(%s) ", printouts[node->type]);
            break;
        case PG_SDF_NODE_MATRIX: {
            int j, k;
            out_i += snprintf(out, end - out, "(%s", printouts[node->type]);
            for(j = 0; j < 4; ++j) for(k = 0; k < 4; ++k) {
                out_i += snprintf(out_i, end - out_i, " %f",
                                  node->matrix[j][k]);
            }
            out_i += snprintf(out_i, end - out_i, ") ");
            break;
        }
        case PG_SDF_NODE_VECTOR:
            out_i += snprintf(out, end - out, "(%s %f %f %f) ",
                printouts[node->type],
                node->vector[0], node->vector[1], node->vector[2]);
            break;
        default: break;
        }
        return out_i;
    } else if(node->type == PG_SDF_NODE_ARGUMENT) {
        out_i += snprintf(out_i, end - out_i, "$%d ", node->arg);
        return out_i;
    } else {
        out_i += snprintf(out_i, end - out_i, "(%s ", printouts[node->type]);
        struct pg_sdf_node* child[2] = {
            pg_sdf_node_child(sdf, node, 0),
            pg_sdf_node_child(sdf, node, 1) };
        out_i = print_node_recursive(sdf, child[0], out_i, end);
        out_i = print_node_recursive(sdf, child[1], out_i, end);
        out_i += snprintf(out_i, end - out_i, ") ");
        return out_i;
    }
    
}

int pg_sdf_tree_print(struct pg_sdf_tree* sdf, char* out, unsigned max)
{
    const struct pg_sdf_node* node = sdf->op_tree.data;
    char* end = out + max;
    char* out_i = print_node_recursive(sdf, node, out, end);
    *out_i = '\0';
    return out_i - out;
}

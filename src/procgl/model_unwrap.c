/*  This model unwrapping pipeline works basically like this:
    1:  Create seams in the model, splitting it into six subdivisions based
        on face normals; this may add new duplicate vertices to the model.
    2:  Split those six subdivisions into more subdivisions (note this is not
        recursive subdivision) based on contiguity.
    3:  For each subdivision at this point, (each of which now hold "patches"
        of vertices, separated by contiguity and normal direction), set each
        vertex's UV coords to a simple orthographic projection of the 3D
        position, along the average normal of the patch. This should
        *approximate* a good UV unwrapping but will have distortion on curved
        surfaces, so...
    4:  For each subdivision, solve the subdivision's vertices as a force-
        directed graph, to adjust the length of the edges of the UV verts to
        match the length of the edges of the 3D verts. Now each face should
        have approximately the same texel density.
    5:  For each subdivision, calculate the sum of the 2D area of each 3D
        triangle; then calculate its area as a fraction of the total area of
        all the subdivisions.
    6:  Use a box-packing algorithm to pack all of the subdivisions' UV
        projections into a single composite UV set.
    7:  Done! Write the newly calculated model UV component to the model.   */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "ext/linmath.h"
#include "arr.h"
#include "wave.h"
#include "sdf.h"
#include "model.h"

#define MAX_CONNECTED_TRIS  (16)

struct patch_tri {
    unsigned model_tri_idx;
    unsigned connected[MAX_CONNECTED_TRIS]; /*  tris connected to this vertex */
    unsigned num_connected;
};

struct patch {
    /*  Array of each vertex in a pg_model, with UV data    */
    struct patch_tri* tris;
    /*  Incomplete array of indices into the tris array; each one of these
        indicates the first element of the next "patch" */
    int* idx;
    int idx_len;
};

static inline int norm_to_dir(vec3 norm)
{
    vec3 norm_abs;
    vec3_abs(norm_abs, norm);
    int max_d = (norm_abs[0] > norm_abs[1]
                 ? (norm_abs[0] > norm_abs[2] ? 0 : 2)
                 : (norm_abs[1] > norm_abs[2] ? 1 : 2));
    return (max_d * 2 + (norm[max_d] > 0));
}

static inline void pos_3d_to_2d(vec2 out, vec3 pos, int dir)
{
    switch(dir) {
    case 0: case 1: vec2_set(out, pos[1], pos[2]); break;
    case 2: case 3: vec2_set(out, pos[0], pos[2]); break;
    case 4: case 5: vec2_set(out, pos[0], pos[1]); break;
    }
}

static inline int edge_intersection(vec2 a0, vec2 a1, vec2 b0, vec2 b1)
{
    float aa = a1[1] - a0[1];
    float ab = a0[0] - a1[0];
    float ac = aa * a0[0] + ab * a0[1];
    float ba = b1[1] - b0[1];
    float bb = b0[0] - b1[0];
    float bc = aa * b0[0] + ab * b0[1];
    float det = aa * ab - ab * bb;
    float x, y;
    if(det == 0){
        return 0;
    } else {
        x = (bb * ac - ab * bc) / det;
        y = (aa * bc - ba * ac) / det;
    }
    if(x >= MIN(a0[0], a1[0]) && x <= MAX(a0[0], a1[0])
    && x >= MIN(b0[0], b1[0]) && x <= MAX(b0[0], b1[0])
    && y >= MIN(a0[1], a1[1]) && y <= MAX(a0[1], a1[1])
    && y >= MIN(b0[1], b1[1]) && y <= MAX(b0[1], b1[1])) return 1;
    else return 0;
}

static inline int pg_model_face_overlap(struct pg_model* model, int dir,
                                        unsigned t0, unsigned t1)
{
    unsigned t[2][3] = {
        { model->tris.data[t0].t[0],
          model->tris.data[t0].t[1],
          model->tris.data[t0].t[2] },
        { model->tris.data[t1].t[0],
          model->tris.data[t1].t[1],
          model->tris.data[t1].t[2] } };
    vec2 tri0[3];
    vec2 tri1[3];
    pos_3d_to_2d(tri0[0], model->pos.data[t[0][0]].v, dir);
    pos_3d_to_2d(tri0[1], model->pos.data[t[0][1]].v, dir);
    pos_3d_to_2d(tri0[2], model->pos.data[t[0][2]].v, dir);
    pos_3d_to_2d(tri1[0], model->pos.data[t[1][0]].v, dir);
    pos_3d_to_2d(tri1[1], model->pos.data[t[1][1]].v, dir);
    pos_3d_to_2d(tri1[2], model->pos.data[t[1][2]].v, dir);
    if(edge_intersection(tri0[0], tri0[1], tri1[0], tri1[1])
    || edge_intersection(tri0[0], tri0[1], tri1[0], tri1[2])
    || edge_intersection(tri0[0], tri0[1], tri1[1], tri1[2])
    || edge_intersection(tri0[0], tri0[2], tri1[0], tri1[1])
    || edge_intersection(tri0[0], tri0[2], tri1[0], tri1[2])
    || edge_intersection(tri0[0], tri0[2], tri1[1], tri1[2])
    || edge_intersection(tri0[1], tri0[2], tri1[0], tri1[1])
    || edge_intersection(tri0[1], tri0[2], tri1[0], tri1[2])
    || edge_intersection(tri0[1], tri0[2], tri1[1], tri1[2])) return 1;
    else return 0;
}

void pg_model_seams_cardinal_directions(struct pg_model* model)
{
    unsigned tris_tmp[MAX_CONNECTED_TRIS];
    int i;
    for(i = 0; i < model->v_count; ++i) {
        int new_dupe = 0;
        int connected = 0;
        int j = 0;
        struct pg_vertex_full vert;
        pg_model_get_vertex(model, &vert, i);
        struct pg_tri* tri;
        ARR_FOREACH_PTR(model->tris, tri, j) {
            int l = (tri->t[0] == i) ? 0 :
                    (tri->t[1] == i) ? 1 :
                    (tri->t[2] == i) ? 2 : 3;
            if(l == 3) continue;
            if(connected < MAX_CONNECTED_TRIS) {
                tris_tmp[connected] = j * 3 + l;
            } else {
                if(i == connected) {
                    new_dupe = pg_model_add_vertex(model, &vert);
                }
                tri->t[l] = new_dupe;
            }
            ++connected;
        }
        connected = MIN(MAX_CONNECTED_TRIS, connected);
        int c_dir[MAX_CONNECTED_TRIS];
        for(j = 0; j < connected; ++j) {
            /*  Get normal for connected triangle (do the calculations again
                here, in case the model's normals are modified in any way)  */
            vec3 norm;
            pg_model_get_face_normal(model, tris_tmp[j] / 3, norm);
            c_dir[j] = norm_to_dir(norm);
        }
        int first = -1;
        int dir_new_idx[6] = { -1, -1, -1, -1, -1, -1 };
        for(j = 0; j < connected; ++j) {
            if(first == -1) {
                first = c_dir[j];
                dir_new_idx[first] = i;
            }
            if(c_dir[j] == first) {
                dir_new_idx[c_dir[j]] = dir_new_idx[first];
            } else if(dir_new_idx[c_dir[j]] == -1) {
                dir_new_idx[c_dir[j]] = pg_model_add_vertex(model, &vert);
            }
            model->tris.data[tris_tmp[j] / 3].t[tris_tmp[j] % 3] =
                dir_new_idx[c_dir[j]];
        }
    }
}

struct tri_queue {
    unsigned* q;
    unsigned max, start, end;
};
#define QUEUE_PUSH(Q, V)  do { \
        Q.q[Q.end] = (V); \
        Q.end = (Q.end + 1) % Q.max; \
    } while(0)
#define QUEUE_POP(Q, I) do { \
        if(q.start == q.end) break; \
        I = Q.q[q.start]; \
        q.start = (q.start + 1) % Q.max; \
    } while(0)

static void pg_model_build_patches(struct pg_model* model,
                                   struct patch* patch)
{
    uint8_t tri_marks[model->tris.len];
    memset(tri_marks, 0, model->tris.len * sizeof(uint8_t));
    int patch_count = 0;      /*  Current patch index */
    int current_patch_end = 0;
    unsigned queue_data[model->tris.len]; /*  Queue storage for the BFS   */
    memset(queue_data, 0, model->tris.len * sizeof(unsigned));
    struct tri_queue q = { queue_data, model->tris.len };
    int i;
    struct pg_tri tri;
    /*  The top level of the breadth-first search; finding the next unmarked
        triangle    */
    ARR_FOREACH(model->tris, tri, i) {
        if(tri_marks[i]) continue;
        /*  Right now the queue should be empty; push the unmarked vert to
            the queue   */
        QUEUE_PUSH(q, i);
        vec3 norm;
        pg_model_get_face_normal(model, i, norm);
        int dir = norm_to_dir(norm);
        /*  Starting with this tri, we continue expanding until the queue is
            empty again */
        while(q.start != q.end) {
            unsigned q_pop;
            QUEUE_POP(q, q_pop);
            if(tri_marks[q_pop]) continue;
            tri_marks[q_pop] = 1;
            struct pg_tri* q_tri = &model->tris.data[q_pop];
            struct patch_tri new_patch_tri = { .model_tri_idx = q_pop };
            int j;
            struct pg_tri* c_tri;
            /*  Find all the tris connected to this one by TWO vertices, and
                which don't overlap (in the 2d projected plane) with any of
                the current patch's other triangles */
            ARR_FOREACH_PTR(model->tris, c_tri, j) {
                if(j == q_pop) continue;
                /*  Count and record the shared vertices    */
                int k, count = 0, shared[3];
                for(k = 0; k < 3; ++k) {
                    if(c_tri->t[k] == q_tri->t[0]
                    || c_tri->t[k] == q_tri->t[1]
                    || c_tri->t[k] == q_tri->t[2]) shared[count++] = c_tri->t[k];
                }
                if(count != 2) continue;
                /*  If it shares two verts, and is already marked, then it
                    must be itself part of the patch so it doesn't need to be
                    recalculated. Just add it to the popped triangle's list */
                if(tri_marks[j]) {
                    new_patch_tri.connected[new_patch_tri.num_connected++] = j;
                    continue;
                }
                /*  Check for overlap with the current patch    */
                for(k = patch->idx[patch_count]; k < current_patch_end; ++k) {
                    struct patch_tri* p_tri = &patch->tris[k];
                    if(pg_model_face_overlap(model, dir, q_pop, p_tri->model_tri_idx)) {
                        /*  Found a triangle which overlaps with the patch...
                            The vertices it shares need to be duplicated, then
                            replaced in all unmarked triangles  */
                        int j;
                        struct pg_tri dupe_tri;
                        struct pg_vertex_full dupe_vert[2];
                        pg_model_get_vertex(model, &dupe_vert[0], shared[0]);
                        pg_model_get_vertex(model, &dupe_vert[1], shared[1]);
                        unsigned dupe_idx[2] = {
                            pg_model_add_vertex(model, &dupe_vert[0]),
                            pg_model_add_vertex(model, &dupe_vert[1]) };
                        ARR_FOREACH(model->tris, dupe_tri, j) {
                            if(tri_marks[j] || j == q_pop) continue;
                            if(dupe_tri.t[0] == shared[0]) dupe_tri.t[0] = dupe_idx[0];
                            else if(dupe_tri.t[1] == shared[0]) dupe_tri.t[1] = dupe_idx[0];
                            else if(dupe_tri.t[2] == shared[0]) dupe_tri.t[2] = dupe_idx[0];
                            if(dupe_tri.t[0] == shared[1]) dupe_tri.t[0] = dupe_idx[1];
                            else if(dupe_tri.t[1] == shared[1]) dupe_tri.t[1] = dupe_idx[1];
                            else if(dupe_tri.t[2] == shared[1]) dupe_tri.t[2] = dupe_idx[1];
                        }
                        continue;
                    }
                }
                /*  Found a connected, non-overlapping triangle! Add it to the
                    queue, and add it to the current popped triangle's list */
                QUEUE_PUSH(q, j);
                new_patch_tri.connected[new_patch_tri.num_connected] = j;
            }
            /*  new_patch_tri should now be filled out with all its connected
                tris, mark it down and move onto the next queued triangle   */
            patch->tris[current_patch_end++] = new_patch_tri;
            tri_marks[q_pop] = 1;
        }
        /*  Now that the queue is empty, we have finished filling in one patch;
            Record the ending index of the completed patch, then find the next
            unmarked triangle to start on the next one  */
        patch->idx[patch_count++] = current_patch_end;
    }
    patch->idx_len = patch_count;
}

void pg_model_precalc_uv(struct pg_model* model)
{
    pg_model_seams_cardinal_directions(model);
    struct patch patch = {
        .tris = calloc(model->tris.len + 1, sizeof(struct patch_tri)),
        .idx = calloc(model->tris.len + 1, sizeof(int)) };
    pg_model_build_patches(model, &patch);
    printf("Precalculated UVs?\n");
    pg_model_reserve_component(model, PG_MODEL_COMPONENT_COLOR);
    vec4 patch_color;
    int tri_idx = 0;
    int patch_end = 0;
    int current_patch = 0;
    while(current_patch < patch.idx_len) {
        vec4_set(patch_color, rand() / (float)RAND_MAX * 0.5 + 0.25,
                              rand() / (float)RAND_MAX * 0.5 + 0.25,
                              rand() / (float)RAND_MAX * 0.5 + 0.25, 1.0);
        patch_end = patch.idx[current_patch++];
        while(tri_idx < patch_end) {
            struct patch_tri* p_tri = &patch.tris[tri_idx++];
            struct pg_tri* m_tri = &model->tris.data[p_tri->model_tri_idx];
            vec4_dup(model->color.data[m_tri->t[0]].v, patch_color);
            vec4_dup(model->color.data[m_tri->t[1]].v, patch_color);
            vec4_dup(model->color.data[m_tri->t[2]].v, patch_color);
        }
    }
}

static void pg_model_solve_uv(struct pg_model* model, struct patch* patch);
static void pg_patch_pack_uv(struct patch* patch, struct pg_model* model);


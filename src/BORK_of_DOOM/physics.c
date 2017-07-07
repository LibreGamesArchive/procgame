#include <math.h>
#include <stdio.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "map_area.h"
#include "physics.h"

static int is_point_in_triangle(vec3 point, vec3 a, vec3 b, vec3 c)
{
    float total_angles = 0.0f;
    // make the 3 vectors
    vec3 v1, v2, v3;
    vec3_sub(v1, point, a);
    vec3_sub(v2, point, b);
    vec3_sub(v3, point, c);
    vec3_normalize(v1, v1);
    vec3_normalize(v2, v2);
    vec3_normalize(v3, v3);
    total_angles += acosf(vec3_mul_inner(v1,v2));   
    total_angles += acosf(vec3_mul_inner(v2,v3));
    total_angles += acosf(vec3_mul_inner(v3,v1)); 
    if (fabs(total_angles - 2 * M_PI) <= 0.05) return 1;
    else return 0;
}

void nearest_on_triangle(vec3 out, vec3 p, vec3 a, vec3 b, vec3 c)
{
    // Check if P in vertex region outside A
    vec3 ab, ac, ap, bp, cp;
    vec3_sub(ab, b, a);
    vec3_sub(ac, c, a);
    vec3_sub(ap, p, a);
    float d1 = vec3_mul_inner(ab, ap);
    float d2 = vec3_mul_inner(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f) {
        vec3_dup(out, a);
        return;
    }
    // Check if P in vertex region outside B
    vec3_sub(bp, p, b);
    float d3 = vec3_mul_inner(ab, bp);
    float d4 = vec3_mul_inner(ac, bp);
    if (d3 >= 0.0f && d4 <= d3) {
        vec3_dup(out, b);
        return; // barycentric coordinates (0,1,0)
    }
    // Check if P in edge region of AB, if so return projection of P onto AB
    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        float v=d1/(d1- d3);
        vec3_dup(out, ab);
        vec3_scale(out, out, v);
        vec3_add(out, out, a);
        return; // barycentric coordinates (1-v,v,0)
    }
    // Check if P in vertex region outside C
    vec3_sub(cp, p, c);
    float d5 = vec3_mul_inner(ab, cp);
    float d6 = vec3_mul_inner(ac, cp);
    if (d6 >= 0.0f && d5 <= d6) {
        vec3_dup(out, c);
        return; // barycentric coordinates (0,0,1)
    }
    // Check if P in edge region of AC, if so return projection of P onto AC
    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        float w=d2/(d2- d6);
        vec3_dup(out, ac);
        vec3_scale(out, out, w);
        vec3_add(out, out, a);
        return; // barycentric coordinates (1-w,0,w)
    }
    // Check if P in edge region of BC, if so return projection of P onto BC
    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        vec3_sub(out, c, b);
        vec3_scale(out, out, w);
        vec3_add(out, out, b);
        return; // barycentric coordinates (0,1-w,w)
    }
    // P inside face region. Compute Q through its barycentric coordinates (u,v,w)
    float denom = 1.0f / (va + vb + vc);
    float v=vb* denom;
    float w=vc* denom;
    vec3_scale(ab, ab, v);
    vec3_scale(ac, ac, w);
    vec3_add(ab, ab, ac);
    vec3_add(out, ab, a);
}

/*  Some utility functions for ray casting  */
static inline float sd_plane(vec3 const p, vec3 const normal, float plane_d)
{
    return vec3_mul_inner(p, normal) + plane_d;
}

/*  Actual collision detection/response below:  */

struct collision_info {
    vec3 face_ix, sphere_ix;
};

static int collide_with_tri(struct collision_info* info, vec3 pos,
                            vec3 t0, vec3 t1, vec3 t2, vec3 tnorm)
{
    vec3 sphere_ix, face_ix;
    float plane_d = -sd_plane(t0, tnorm, 0);
    float dist_to_plane = sd_plane(pos, tnorm, plane_d);
    if(dist_to_plane > 1) return 0;
    else if(dist_to_plane < 0) return 0;
    vec3_set(face_ix, pos[0] - dist_to_plane * tnorm[0],
                      pos[1] - dist_to_plane * tnorm[1],
                      pos[2] - dist_to_plane * tnorm[2]);
    if(is_point_in_triangle(face_ix, t0, t1, t2)) {
        vec3_sub(info->sphere_ix, pos, tnorm);
        vec3_dup(info->face_ix, face_ix);
        return 1;
    }
    nearest_on_triangle(face_ix, pos, t0, t1, t2);
    vec3_sub(sphere_ix, face_ix, pos);
    float dist = vec3_len(sphere_ix);
    if(dist > 1) return 0;
    vec3_normalize(sphere_ix, sphere_ix);
    vec3_add(sphere_ix, sphere_ix, pos);
    vec3_dup(info->face_ix, face_ix);
    vec3_dup(info->sphere_ix, sphere_ix);
    return 1;
}

/*  Finds the triangle which has the deepest collision into the ellipsoid,
    and returns information about the collision through 'coll_out', while
    returning 1 if there is a collision, 0 otherwise    */
int bork_map_collide(struct bork_map* map, struct bork_collision* coll_out,
                     vec3 out, vec3 _pos, vec3 size)
{
    /*   The first thing we do is scale the player and his velocity to
         ellipsoid space    */
    vec3 pos;
    vec3_div(pos, _pos, size);
    /*  Now make the box to check in the map for collisions, also in
        ellipsoid space; also scaled up a little bit so there's a safe
        buffer area between the ellipsoid and the map which is also checked */
    vec3 size_scaled;
    vec3_scale(size_scaled, size, 1.25);
    box bbox;
    vec3_sub(bbox[0], _pos, size_scaled);
    vec3_add(bbox[1], _pos, size_scaled);
    box_mul_vec3(bbox, bbox, (vec3){ 0.5, 0.5, 0.5 });
    int check[2][3] = { { (int)bbox[0][0]-1, (int)bbox[0][1]-1, (int)bbox[0][2]-1 },
                        { (int)bbox[1][0]+1, (int)bbox[1][1]+1, (int)bbox[1][2]+1 } };
    /*  Set up the return struct and push accumulation variables for the
        collision function  */
    vec3 push = {};
    struct collision_info info = {};
    int x, y, z;
    int count = 0;
    float deepest = 0;
    for(x = check[0][0]; x < check[1][0]; ++x) {
        for(y = check[0][1]; y < check[1][1]; ++y) {
            for(z = check[0][2]; z < check[1][2]; ++z) {
                enum bork_area area = bork_map_get_area(map, x, y, z);
                int idx[3] = {
                    x - map->area_pos[area][0],
                    y - map->area_pos[area][1],
                    z - map->area_pos[area][2] };
                vec3 area_pos = { map->area_pos[area][0], map->area_pos[area][1], map->area_pos[area][2] };
                struct bork_tile* tile = bork_map_tile_ptr(map, area, idx[0], idx[1], idx[2]);
                /*  If the tile is outside the map, or the tile is not
                    collidable, move on to the next one */
                if(!tile || tile->type < 2) continue;
                struct pg_model* model = &map->area_model[area];
                unsigned i;
                for(i = 0; i < tile->num_tris; ++i) {
                    struct pg_tri* tri = &model->tris.data[i + tile->model_tri_idx];
                    vec3 p0, p1, p2;
                    vec3_dup(p0, model->pos.data[tri->t[0]].v);
                    vec3_dup(p1, model->pos.data[tri->t[1]].v);
                    vec3_dup(p2, model->pos.data[tri->t[2]].v);
                    vec3_add(p0, p0, area_pos);
                    vec3_add(p1, p1, area_pos);
                    vec3_add(p2, p2, area_pos);
                    vec3_div(p0, p0, size);
                    vec3_div(p1, p1, size);
                    vec3_div(p2, p2, size);
                    vec3 plane_1, plane_2;
                    vec3_sub(plane_1, p1, p0);
                    vec3_sub(plane_2, p2, p0);
                    vec3 norm;
                    vec3_wedge(norm, plane_1, plane_2);
                    vec3_normalize(norm, norm);
                    int c = collide_with_tri(&info, pos, p0, p1, p2, norm);
                    if(c) {
                        vec3 tri_push;
                        vec3_sub(tri_push, info.face_ix, info.sphere_ix);
                        /*  If this collision has the greatest depth, then
                            this is the one we will use */
                        float depth = vec3_len(tri_push);
                        if(depth <= deepest) continue;
                        ++count;
                        deepest = depth;
                        vec3_dup(push, tri_push);
                        *coll_out = (struct bork_collision) {
                            .x = x, .y = y, .z = z,
                            .norm = { norm[0], norm[1], norm[2] },
                            .tile = tile };
                    }
                }
            }
        }
    }
    /*  Set the new position to the final pushed position   */
    vec3_add(out, pos, push);
    /*  Convert back from ellipsoid space, and we're done   */
    vec3_mul(out, out, size);
    return (!!count);
}

#include <math.h>
#include <stdio.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "physics.h"

/*  Finds the triangle which has the deepest collision into the ellipsoid,
    and returns information about the collision through 'coll_out', while
    returning 1 if there is a collision, 0 otherwise    */
int bork_map_collide(struct bork_map* map, struct bork_collision* coll_out,
                     vec3 const pos, vec3 const size)
{
    /*  Now make the box to check in the map for collisions, also in
        ellipsoid space; also scaled up a little bit so there's a safe
        buffer area between the ellipsoid and the map which is also checked */
    mat4 transform;
    vec3 size_scaled;
    vec3_scale(size_scaled, size, 1.5);
    box bbox;
    vec3_sub(bbox[0], pos, size_scaled);
    vec3_add(bbox[1], pos, size_scaled);
    box_mul_vec3(bbox, bbox, (vec3){ 0.5, 0.5, 0.5 });
    int check[2][3] = { { (int)bbox[0][0], (int)bbox[0][1], (int)bbox[0][2] },
                        { (int)bbox[1][0], (int)bbox[1][1], (int)bbox[1][2] } };
    /*  First check collisions against the map  */
    struct pg_model* model = &map->model;
    int x, y, z;
    int hits = 0;
    float deepest = 0;
    for(z = check[0][2]; z <= check[1][2]; ++z) {
        for(y = check[0][1]; y <= check[1][1]; ++y) {
            for(x = check[0][0]; x <= check[1][0]; ++x) {
                /*  Get the map area that the checked box is in */
                /*  Get a pointer to the tile in the map area   */
                struct bork_tile* tile = bork_map_tile_ptri(map, x, y, z);
                /*  If the tile is outside the map, or the tile is not
                    collidable, move on to the next one */
                if(!tile || tile->type < 2) continue;
                /*  Otherwise test collisions against this tile's associated
                    triangles in the area model */
                vec3 tile_push, tile_norm;
                int c = pg_model_collide_ellipsoid_sub(model, tile_push,
                            pos, size, 3, tile->model_tri_idx, tile->num_tris);
                if(c < 0) continue;
                pg_model_get_face_normal(model, c, tile_norm);
                float depth = vec3_len(tile_push);
                if(depth <= deepest) continue;
                deepest = depth;
                *coll_out = (struct bork_collision) {
                    .x = x, .y = y, .z = z, .tile = tile };
                vec3_dup(coll_out->push, tile_push);
                vec3_dup(coll_out->face_norm, tile_norm);
                if(++hits > 3) return 1;
            }
        }
    }
    /*  Next check collisions against map objects (that are close enough)   */
    struct bork_map_object* obj;
    int i;
    ARR_FOREACH_PTR(map->objects, obj, i) {
        if(obj->type != BORK_MAP_OBJ_DOOR) continue;
        mat4_translate(transform,
            obj->x * 2.0f + 1.0f,
            obj->y * 2.0f + 1.0f,
            obj->z * 2.0f + 1.0f + obj->door.pos);
        if(obj->door.dir) mat4_rotate_Z(transform, transform, M_PI * 0.5);
        mat4 tx_inv;
        mat4_invert(tx_inv, transform);
        vec4 pos_tx;
        mat4_mul_vec4(pos_tx, tx_inv, (vec4){ pos[0], pos[1], pos[2], 1 });
        vec3 obj_push, obj_norm;
        int c = pg_model_collide_ellipsoid(&map->door_model, obj_push, pos_tx, size, 1);
        if(c < 0) continue;
        mat3_mul_vec3(obj_push, transform, obj_push);
        pg_model_get_face_normal(&map->door_model, c, obj_norm);
        float depth = vec3_len(obj_push);
        if(depth <= deepest) continue;
        deepest = depth;
        *coll_out = (struct bork_collision) {
            .x = obj->x, .y = obj->y, .z = obj->z, .tile = NULL };
        vec3_dup(coll_out->push, obj_push);
        vec3_dup(coll_out->face_norm, obj_norm);
        if(++hits > 3) return 1;
    }
    /*  Set the new position to the final pushed position   */
    //vec3_add(out, pos, push);
    return (deepest > 0);
}



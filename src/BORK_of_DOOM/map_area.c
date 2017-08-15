#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "game_states.h"

static const char* BORK_AREA_STRING[] = {
    [BORK_AREA_PETS] = "P.E.T.S.",
    [BORK_AREA_WAREHOUSE] = "WAREHOUSE",
    [BORK_AREA_CAFETERIA] = "CAFETERIA",
    [BORK_AREA_REC_CENTER] = "DOG PARK",
    [BORK_AREA_INFIRMARY] = "VETERINARY CLINIC",
    [BORK_AREA_SCIENCE_LABS] = "SCIENCE LABS",
    [BORK_AREA_COMMAND] = "COMMAND CENTER",
    [BORK_AREA_MUTT] = "M.U.T.T.",
    [BORK_AREA_EXTERIOR] = "BONZ EXTERIOR",
};
const char* bork_map_area_str(enum bork_area area) {
    if(area >= BORK_AREA_PETS && area <= BORK_AREA_MUTT) return BORK_AREA_STRING[area];
    else return BORK_AREA_STRING[BORK_AREA_EXTERIOR];
}

#define BORK_AREA_FLAG_SQUARE   (1 << 0)

static int tile_model_basic(struct bork_map*, struct pg_texture*,
                            struct bork_tile*, int, int, int);
static void bork_map_generate_model(struct bork_map* map,
                                    struct pg_texture* env_atlas);

/*  Tile details */
struct bork_tile_detail BORK_TILE_DETAILS[] = {
    [BORK_TILE_VAC] = { .name = "Vacuum" },
    [BORK_TILE_ATMO] = { .name = "Atmosphere" },
    [BORK_TILE_HULL] = { .name = "Basic hull",
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [PG_LEFT] = 3, [PG_RIGHT] = 3,
            [PG_FRONT] = 3, [PG_BACK] = 3,
            [PG_TOP] = 3, [PG_DOWN] = 3 },
        .add_model = tile_model_basic },
    [BORK_TILE_LADDER] = { .name = "Ladder",
        .tile_flags = BORK_TILE_HAS_ORIENTATION | BORK_TILE_FACE_ORIENTED,
        .face_flags = { BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE,
                        0, 0 },
        .face_inset = { 0.1, 0.1, 0.1, 0.1 },
        .tex_tile = {
            [PG_LEFT] = 5, [PG_RIGHT] = 5,
            [PG_FRONT] = 5, [PG_BACK] = 5 },
        .add_model = tile_model_basic },
    [BORK_TILE_CATWALK] = { .name = "Catwalk",
        .face_flags = { [PG_TOP] = BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE },
        .face_inset = { },
        .tex_tile = { [PG_TOP] = 6 },
        .add_model = tile_model_basic },
    [BORK_TILE_HANDRAIL] = { .name = "Handrail",
        .tile_flags = BORK_TILE_HAS_ORIENTATION | BORK_TILE_FACE_ORIENTED,
        .face_flags = { BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM },
        .face_inset = { -0.05, -0.05, -0.05, -0.05 },
        .tex_tile = { 7, 7, 7, 7 },
        .add_model = tile_model_basic },
    [BORK_TILE_EDITOR_DOOR] = { .name = "Door (-> Atmosphere)",
        .tile_flags = BORK_TILE_HAS_ORIENTATION,
    },
    [BORK_TILE_EDITOR_LIGHT1] = { .name = "Light #1 (-> Atmosphere)" },
    [BORK_TILE_EDITOR_LIGHT_WALLMOUNT] = { .name = "WALL-MOUNTED LIGHT",
        .tile_flags = BORK_TILE_HAS_ORIENTATION },
};

const struct bork_tile_detail* bork_tile_detail(enum bork_tile_type type)
{
    if(type >= BORK_TILE_COUNT) return &BORK_TILE_DETAILS[BORK_TILE_VAC];
    return &BORK_TILE_DETAILS[type];
}

/*  Public interface    */
void bork_map_init(struct bork_map* map)
{
    *map = (struct bork_map){};
}

void bork_map_init_model(struct bork_map* map, struct bork_game_core* core)
{
    /*  Generate the map model  */
    pg_model_init(&map->model);
    bork_map_generate_model(map, &core->env_atlas);
    pg_shader_buffer_model(&core->shader_3d, &map->model);
    /*  And the door model  */
    pg_model_init(&map->door_model);
    int i;
    vec4 face_uv[6];
    for(i = 0; i < 6; ++i) {
        pg_texture_get_frame(&core->env_atlas, 2, face_uv[i], face_uv[i] + 2);
    }
    pg_model_rect_prism(&map->door_model, (vec3){ 1, 0.1, 1 }, face_uv);
    pg_model_precalc_ntb(&map->door_model);
    pg_shader_buffer_model(&core->shader_3d, &map->door_model);
}

void bork_map_deinit(struct bork_map* map)
{
    pg_model_deinit(&map->model);
    pg_model_deinit(&map->door_model);
    ARR_DEINIT(map->objects);
    ARR_DEINIT(map->enemies);
    ARR_DEINIT(map->items);
}

void bork_map_update(struct bork_map* map, struct bork_entity* plr)
{
    struct bork_map_object* obj;
    int i;
    ARR_FOREACH_PTR(map->objects, obj, i) {
        if(obj->type == BORK_MAP_OBJ_DOOR) {
            vec3 obj_pos = { obj->x * 2.0f + 1.0f,
                             obj->y * 2.0f + 1.0f,
                             obj->z * 2.0f + 1.0f };
            vec3 plr_to_door;
            vec3_sub(plr_to_door, obj_pos, plr->pos);
            float dist = vec3_len(plr_to_door);
            if(dist < 3) {
                obj->door.pos += 0.1;
                if(obj->door.pos > 2) obj->door.pos = 2;
            } else if(obj->door.pos > 0) {
                obj->door.pos -= 0.1;
                if(obj->door.pos < 0) obj->door.pos = 0;
            }
        }
    }
}

void bork_map_draw(struct bork_map* map, struct bork_game_core* core)
{
    pg_shader_begin(&core->shader_3d, &core->view);
    /*  Draw the base level geometry    */
    pg_model_begin(&map->model, &core->shader_3d);
    mat4 model_transform;
    mat4_translate(model_transform, 0, 0, 0);
    pg_model_draw(&map->model, model_transform);
    /*  Then draw the map objects (except lights)   */
    int i;
    struct bork_map_object* obj;
    pg_model_begin(&map->door_model, &core->shader_3d);
    ARR_FOREACH_PTR(map->objects, obj, i) {
        if(obj->type == BORK_MAP_OBJ_DOOR) {
            mat4_translate(model_transform,
                           obj->x * 2.0f + 1.0f,
                           obj->y * 2.0f + 1.0f,
                           obj->z * 2.0f + 1.0f + obj->door.pos);
            if(obj->door.dir) mat4_rotate_Z(model_transform, model_transform, M_PI * 0.5);
            pg_model_draw(&map->door_model, model_transform);
        }
    }
}

struct bork_tile* bork_map_tile_ptr(struct bork_map* map, vec3 const pos)
{
    int x = (int)(pos[0] * 0.5f);
    int y = (int)(pos[1] * 0.5f);
    int z = (int)(pos[2] * 0.5f);
    if(x >= 0 && x < 32 && y >= 0 && y < 32 && z >= 0 && z < 32)
        return &map->data[x][y][z];
    else return NULL;
}

struct bork_tile* bork_map_tile_ptri(struct bork_map* map, int x, int y, int z)
{
    if(x >= 0 && x < 32 && y >= 0 && y < 32 && z >= 0 && z < 32)
        return &map->data[x][y][z];
    else return NULL;
}

int bork_map_check_sphere(struct bork_map* map, vec3 const pos, float r)
{
    box bbox;
    vec3 r_scaled = { r * 1.25, r * 1.25, r * 1.25 };
    vec3_sub(bbox[0], pos, r_scaled);
    vec3_add(bbox[1], pos, r_scaled);
    box_mul_vec3(bbox, bbox, (vec3){ 0.5, 0.5, 0.5 });
    int check[2][3] = { { (int)bbox[0][0], (int)bbox[0][1], (int)bbox[0][2] },
                        { (int)bbox[1][0], (int)bbox[1][1], (int)bbox[1][2] } };
    /*  First check collisions against the map  */
    struct pg_model* model = &map->model;
    int x, y, z;
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
                vec3 tile_push;
                int c = pg_model_collide_sphere_sub(model, tile_push,
                            pos, r, 1, tile->model_tri_idx, tile->num_tris);
                if(c >= 0) return 1;
            }
        }
    }
    return 0;
}

int bork_map_check_vis(struct bork_map* map, vec3 const start, vec3 const end)
{
    vec3 full_vec, part_vec, curr_point;
    vec3_sub(full_vec, end, start);
    float full_dist = vec3_len(full_vec);
    float part_dist = 0.5;
    float curr_dist = 0;
    vec3_set_len(part_vec, full_vec, part_dist);
    vec3_dup(curr_point, start);
    while(curr_dist <= full_dist) {
        if(bork_map_check_sphere(map, curr_point, 0.25)) return 0;
        if(curr_dist + part_dist > full_dist) vec3_dup(curr_point, end);
        else vec3_add(curr_point, curr_point, part_vec);
        curr_dist += part_dist;
    }
    return 1;
}

/*  Model generation code   */

static void bork_map_generate_model(struct bork_map* map, struct pg_texture* env_atlas)
{
    pg_model_init(&map->model);
    map->model.components = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    struct bork_tile* tile;
    int x, y, z;
    for(x = 0; x < 32; ++x) {
        for(y = 0; y < 32; ++y) {
            for(z = 0; z < 32; ++z) {
                tile = &map->data[x][y][z];
                if(!tile || tile->type < 2) continue;
                struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
                tile->model_tri_idx = map->model.tris.len;
                tile->num_tris = detail->add_model(map, env_atlas, tile, x, y, z);
            }
        }
    }
    pg_model_precalc_ntb(&map->model);
}

/*  Generating geometry for individual tiles    */
/*  The BASIC tile geometry generation; just variations on a cube   */
static const vec3 vert_pos[6][4] = {
    { { 0.5, 0.5, -0.5 },
      { 0.5, 0.5, 0.5 },
      { -0.5, 0.5, -0.5 },
      { -0.5, 0.5, 0.5 } },
    { { -0.5, -0.5, -0.5 },
      { -0.5, -0.5, 0.5 },
      { 0.5, -0.5, -0.5 },
      { 0.5, -0.5, 0.5 } },
    { { 0.5, -0.5, -0.5 },
      { 0.5, -0.5, 0.5 },
      { 0.5, 0.5, -0.5 },
      { 0.5, 0.5, 0.5 } },
    { { -0.5, 0.5, -0.5 },
      { -0.5, 0.5, 0.5 },
      { -0.5, -0.5, -0.5 },
      { -0.5, -0.5, 0.5 } },
    { { -0.5, -0.5, 0.5 },
      { -0.5, 0.5, 0.5 },
      { 0.5, -0.5, 0.5 },
      { 0.5, 0.5, 0.5 } },
    { { -0.5, -0.5, -0.5 },
      { 0.5, -0.5, -0.5 },
      { -0.5, 0.5, -0.5 },
      { 0.5, 0.5, -0.5 } } };

static int tile_face_basic(struct bork_map* map, struct pg_texture* env_atlas,
                           struct bork_tile* tile, int x, int y, int z,
                           enum pg_direction dir)
{
    /*  Get the details for this face   */
    struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
    uint32_t tile_flags = detail->tile_flags;
    uint32_t face_flags = detail->face_flags[dir];
    if(!(face_flags & BORK_FACE_HAS_SURFACE)) return 0; /*  Tile has no face here   */
    /*  Get details for the opposing face   */
    int opp[3] = { x + PG_DIR_VEC[dir][0], y + PG_DIR_VEC[dir][1], z + PG_DIR_VEC[dir][2] };
    struct bork_tile* opp_tile = bork_map_tile_ptri(map, opp[0], opp[1], opp[2]);
    struct bork_tile_detail* opp_detail = opp_tile ?
        &BORK_TILE_DETAILS[opp_tile->type] : &BORK_TILE_DETAILS[0];
    uint32_t opp_flags = opp_detail->face_flags[PG_DIR_OPPOSITE[dir]];
    /*  Decide if this face of the tile needs to be generated   */
    if(opp_flags & BORK_FACE_HAS_SURFACE && !(opp_flags & BORK_FACE_SEETHRU_SURFACE)
    && !(face_flags & BORK_FACE_FORCE_SURFACE) && !(face_flags & BORK_FACE_FLUSH_SURFACE)) {
        return 0;
    } else if(face_flags & BORK_FACE_NO_SELF_OPPOSITE
            && opp_flags & BORK_FACE_NO_SELF_OPPOSITE) {
        return 0;
    } else if((tile_flags & BORK_TILE_FACE_ORIENTED) && !(tile->orientation & (1 << dir))) {
        return 0;
    }
    struct pg_model* model = &map->model;
    /*  Calculate the base info for this face   */
    int num_tris = 0;
    vec3 inset_dir;
    vec3_scale(inset_dir, PG_DIR_VEC[dir], detail->face_inset[dir]);
    vec2 tex_frame[2];
    pg_texture_get_frame(env_atlas, detail->tex_tile[dir],
                         tex_frame[0], tex_frame[1]);
    unsigned vert_idx = model->v_count;
    /*  Generate the geometry   */
    struct pg_vertex_full new_vert = { .components =
        PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV };
    int i;
    if(!(face_flags & BORK_FACE_NO_FRONTFACE)) {
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(i < 2)][0], tex_frame[(i % 2)][1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(face_flags & BORK_FACE_HALF_BOTTOM && new_vert.pos[2] == 0.5) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] += (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 0, vert_idx + 2);
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 2, vert_idx + 3);
        num_tris += 2;
    }
    if(face_flags & BORK_FACE_HAS_BACKFACE) {
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(i < 2)][0], tex_frame[(i % 2)][1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(face_flags & BORK_FACE_HALF_BOTTOM && new_vert.pos[2] == 0.5) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] += (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 4, vert_idx + 5, vert_idx + 6);
        pg_model_add_triangle(model, vert_idx + 6, vert_idx + 5, vert_idx + 7);
        num_tris += 2;
    }
    return num_tris;
}

static int tile_model_basic(struct bork_map* map, struct pg_texture* env_atlas,
                            struct bork_tile* tile, int x, int y, int z)
{
    int tri_count = 0;
    int s;
    for(s = 0; s < 6; ++s) {
        tri_count += tile_face_basic(map, env_atlas, tile, x, y, z, s);
    }
    return tri_count;
}

int bork_map_load_editor_map(struct bork_map* map, char* filename)
{
    struct bork_editor_map ed_map;
    int loaded = bork_editor_load_map(&ed_map, filename);
    if(!loaded) return 0;
    else bork_editor_complete_map(map, &ed_map);
    return 1;
}

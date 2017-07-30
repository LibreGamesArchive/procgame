#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

#define BORK_AREA_FLAG_SQUARE   (1 << 0)

/*  Area generation range definitions   */
static struct bork_area_detail {
    uint32_t flags;
    int w[2];   /*  range of lateral sizes in tiles */
    int h[2];   /*  range of heights in FLOORS (2-3 tile height)    */
} BORK_AREA_DETAILS[] = {
    [BORK_AREA_PETS] = {            .w = { 12, 16 }, .h = { 3, 5 },
        .flags = BORK_AREA_FLAG_SQUARE },
    [BORK_AREA_WAREHOUSE] = {       .w = { 26, 30 }, .h = { 4, 6 } },
    [BORK_AREA_CAFETERIA] = {       .w = { 20, 24 }, .h = { 4, 6 } },
    [BORK_AREA_REC_CENTER] = {      .w = { 20, 24 }, .h = { 2, 3 } },
    [BORK_AREA_INFIRMARY] = {       .w = { 14, 18 }, .h = { 2, 3 } },
    [BORK_AREA_SCIENCE_LABS] = {    .w = { 14, 18 }, .h = { 2, 3 } },
    [BORK_AREA_COMMAND] = {         .w = { 12, 16 }, .h = { 2, 3 },
        .flags = BORK_AREA_FLAG_SQUARE }
};

/*  Tile model generation function declarations */
static void bork_map_generate_area_model(struct bork_map* map, enum bork_area area);
static int tile_model_basic(struct bork_map*, enum bork_area, struct bork_tile*, int, int, int);

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
};

const struct bork_tile_detail* bork_tile_detail(enum bork_tile_type type)
{
    if(type >= BORK_TILE_COUNT) return &BORK_TILE_DETAILS[BORK_TILE_VAC];
    return &BORK_TILE_DETAILS[type];
}

/*  Public interface    */
void bork_map_init(struct bork_map* map, struct bork_game_core* core)
{
    map->core = core;
    int i;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        map->area_pos[i][0] = 0;
        map->area_pos[i][1] = 0;
        map->area_pos[i][2] = i * 32;
        map->data[i] = calloc(32 * 32 * 32, sizeof(struct bork_tile));
        ARR_INIT(map->objects[i]);
        ARR_INIT(map->enemies[i]);
        ARR_INIT(map->items[i]);
    }
}

void bork_map_init_model(struct bork_map* map)
{
    int i;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        pg_model_init(&map->area_model[i]);
        bork_map_generate_area_model(map, i);
        pg_shader_buffer_model(&map->core->shader_3d, &map->area_model[i]);
    }
    pg_model_init(&map->door_model);
    vec4 face_uv[6];
    for(i = 0; i < 6; ++i) {
        pg_texture_get_frame(&map->core->env_atlas, 2, face_uv[i], face_uv[i] + 2);
    }
    pg_model_rect_prism(&map->door_model, (vec3){ 1, 0.1, 1 }, face_uv);
    pg_model_precalc_ntb(&map->door_model);
    pg_shader_buffer_model(&map->core->shader_3d, &map->door_model);
}

void bork_map_deinit(struct bork_map* map)
{
    int i;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        free(map->data[i]);
        pg_model_deinit(&map->area_model[i]);
    }
}

void bork_map_update_area(struct bork_map* map, enum bork_area area,
                          struct bork_entity* plr)
{
    struct bork_map_object* obj;
    int i;
    ARR_FOREACH_PTR(map->objects[area], obj, i) {
        if(obj->type == BORK_MAP_OBJ_DOOR) {
            vec3 obj_pos = {
                (map->area_pos[area][0] + obj->x) * 2.0f + 1.0f,
                (map->area_pos[area][1] + obj->y) * 2.0f + 1.0f,
                (map->area_pos[area][2] + obj->z) * 2.0f + 1.0f };
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
    struct bork_entity* ent;
    ARR_FOREACH_PTR_REV(map->enemies[area], ent, i) {
        if(ent->dead) {
            ARR_SWAPSPLICE(map->enemies[area], i, 1);
            continue;
        }
        bork_entity_update(ent, map);
    }
    ARR_FOREACH_PTR_REV(map->items[area], ent, i) {
        if(ent->dead) {
            ARR_SWAPSPLICE(map->items[area], i, 1);
            continue;
        }
        bork_entity_update(ent, map);
    }
}

void bork_map_draw_area(struct bork_map* map, enum bork_area area)
{
    pg_shader_begin(&map->core->shader_3d, &map->core->view);
    /*  Draw the base level geometry    */
    pg_model_begin(&map->area_model[area], &map->core->shader_3d);
    mat4 model_transform;
    mat4_translate(model_transform,
                   map->area_pos[area][0] * 2.0f,
                   map->area_pos[area][1] * 2.0f,
                   map->area_pos[area][2] * 2.0f);
    pg_model_draw(&map->area_model[area], model_transform);
    /*  Then draw the map objects (except lights)   */
    int i;
    struct bork_map_object* obj;
    pg_model_begin(&map->door_model, &map->core->shader_3d);
    ARR_FOREACH_PTR(map->objects[area], obj, i) {
        if(obj->type == BORK_MAP_OBJ_DOOR) {
            mat4_translate(model_transform,
                (map->area_pos[area][0] + obj->x) * 2.0f + 1.0f,
                (map->area_pos[area][1] + obj->y) * 2.0f + 1.0f,
                (map->area_pos[area][2] + obj->z) * 2.0f + 1.0f + obj->door.pos);
            if(obj->door.dir) mat4_rotate_Z(model_transform, model_transform, M_PI * 0.5);
            pg_model_draw(&map->door_model, model_transform);
        }
    }
}

enum bork_area bork_map_get_area(struct bork_map* map, int x, int y, int z)
{
    int i;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        if(x >= map->area_pos[i][0] && x < map->area_pos[i][0] + 32
        && y >= map->area_pos[i][1] && y < map->area_pos[i][1] + 32
        && z >= map->area_pos[i][2] && z < map->area_pos[i][2] + 32)
            return i;
    }
    return BORK_AREA_EXTERIOR;
}

struct bork_tile* bork_map_tile_ptr(struct bork_map* map, enum bork_area area,
                                    int x, int y, int z)
{
    if(x >= 0 && x < 32 && y >= 0 && y < 32 && z >= 0 && z < 32)
        return &map->data[area][x + y * 32 + z * 32 * 32];
    else return NULL;
}

void bork_map_write_to_file(struct bork_map* map, char* filename)
{
    FILE* file = fopen(filename, "w");
    if(!file) {
        printf("BORK map writing error: could not open file %s\n", filename);
    }
    int area;
    for(area = 0; area < BORK_AREA_EXTERIOR; ++area) {
        fwrite(map->data[area], sizeof(struct bork_tile), 32 * 32 *32, file);
    }
    fclose(file);
}

void bork_map_load_from_file(struct bork_map* map, char* filename)
{
    FILE* file = fopen(filename, "r");
    if(!file) {
        printf("BORK map loading error: could not open file %s\n", filename);
    }
    int area, x, y, z;
    for(area = 0; area < BORK_AREA_EXTERIOR; ++area) {
        fread(map->data[area], sizeof(struct bork_tile), 32 * 32 * 32, file);
        for(x = 0; x < 32; ++x) {
            for(y = 0; y < 32; ++y) {
                for(z = 0; z < 32; ++z) {
                    struct bork_tile* tile = bork_map_tile_ptr(map, area, x, y, z);
                    if(tile->type == BORK_TILE_EDITOR_DOOR) {
                        struct bork_map_object new_obj = {
                            .type = BORK_MAP_OBJ_DOOR, .x = x, .y = y, .z = z };
                        if(tile->orientation & ((1 << PG_LEFT) | (1 << PG_RIGHT))) new_obj.door.dir = 1;
                        ARR_PUSH(map->objects[area], new_obj);
                        tile->type = BORK_TILE_ATMO;
                    } else if(tile->type == BORK_TILE_EDITOR_LIGHT1) {
                        struct bork_map_object new_obj = {
                            .type = BORK_MAP_OBJ_LIGHT,
                            .light = { 1, 1, 1, 8 },
                            .x = x, .y = y, .z = z };
                        ARR_PUSH(map->objects[area], new_obj);
                        tile->type = BORK_TILE_ATMO;
                    }
                }
            }
        }
    }
    fclose(file);
}

/*  Model generation code   */

static void bork_map_generate_area_model(struct bork_map* map, enum bork_area area)
{
    pg_model_reset(&map->area_model[area]);
    map->area_model[area].components = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    struct bork_tile* tile;
    int x, y, z;
    for(x = 0; x < 32; ++x) {
        for(y = 0; y < 32; ++y) {
            for(z = 0; z < 32; ++z) {
                tile = bork_map_tile_ptr(map, area, x, y, z);
                if(!tile || tile->type < 2) continue;
                struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
                tile->model_tri_idx = map->area_model[area].tris.len;
                tile->num_tris = detail->add_model(map, area, tile, x, y, z);
            }
        }
    }
    pg_model_precalc_ntb(&map->area_model[area]);
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

static int tile_face_basic(struct bork_map* map, enum bork_area area,
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
    struct bork_tile* opp_tile = bork_map_tile_ptr(map, area, opp[0], opp[1], opp[2]);
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
    struct pg_model* model = &map->area_model[area];
    /*  Calculate the base info for this face   */
    int num_tris = 0;
    vec3 inset_dir;
    vec3_scale(inset_dir, PG_DIR_VEC[dir], detail->face_inset[dir]);
    vec2 tex_frame[2];
    pg_texture_get_frame(&map->core->env_atlas, detail->tex_tile[dir],
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

static int tile_model_basic(struct bork_map* map, enum bork_area area,
                            struct bork_tile* tile, int x, int y, int z)
{
    int tri_count = 0;
    int s;
    for(s = 0; s < 6; ++s) {
        tri_count += tile_face_basic(map, area, tile, x, y, z, s);
    }
    return tri_count;
}


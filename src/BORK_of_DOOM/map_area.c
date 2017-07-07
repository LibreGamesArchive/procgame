#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
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
    [BORK_TILE_VAC] = {},
    [BORK_TILE_ATMO] = {},
    [BORK_TILE_HULL] = {
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [PG_LEFT] = 3, [PG_RIGHT] = 3,
            [PG_FRONT] = 3, [PG_BACK] = 3,
            [PG_TOP] = 3, [PG_DOWN] = 3 },
        .add_model = tile_model_basic },
    [BORK_TILE_LADDER] = {
        .face_flags = { BORK_TILE_HAS_SURFACE | BORK_TILE_FLUSH_SURFACE |
                            BORK_TILE_HAS_BACKFACE | BORK_TILE_SEETHRU_SURFACE,
                        BORK_TILE_HAS_SURFACE | BORK_TILE_FLUSH_SURFACE |
                            BORK_TILE_HAS_BACKFACE | BORK_TILE_SEETHRU_SURFACE,
                        BORK_TILE_HAS_SURFACE | BORK_TILE_FLUSH_SURFACE |
                            BORK_TILE_HAS_BACKFACE | BORK_TILE_SEETHRU_SURFACE,
                        BORK_TILE_HAS_SURFACE | BORK_TILE_FLUSH_SURFACE |
                            BORK_TILE_HAS_BACKFACE | BORK_TILE_SEETHRU_SURFACE,
                        0, 0 },
        .face_inset = { 0.2, 0.2, 0.2, 0.2 },
        .tex_tile = {
            [PG_LEFT] = 5, [PG_RIGHT] = 5,
            [PG_FRONT] = 5, [PG_BACK] = 5 },
        .add_model = tile_model_basic },
    [BORK_TILE_CATWALK] = {
        .face_flags = { BORK_TILE_HAS_SURFACE | BORK_TILE_HAS_BACKFACE |
                            BORK_TILE_SEETHRU_SURFACE | BORK_TILE_NO_SELF_OPPOSITE,
                        BORK_TILE_HAS_SURFACE | BORK_TILE_HAS_BACKFACE |
                            BORK_TILE_SEETHRU_SURFACE | BORK_TILE_NO_SELF_OPPOSITE,
                        BORK_TILE_HAS_SURFACE | BORK_TILE_HAS_BACKFACE |
                            BORK_TILE_SEETHRU_SURFACE | BORK_TILE_NO_SELF_OPPOSITE,
                        BORK_TILE_HAS_SURFACE | BORK_TILE_HAS_BACKFACE |
                            BORK_TILE_SEETHRU_SURFACE | BORK_TILE_NO_SELF_OPPOSITE,
                        0, BORK_TILE_HAS_SURFACE | BORK_TILE_HAS_BACKFACE },
        .face_inset = { 0.1, 0.1, 0.1, 0.1 , 0.1, 0.1 },
        .tex_tile = {
            [PG_LEFT] = 7, [PG_RIGHT] = 7,
            [PG_FRONT] = 7, [PG_BACK] = 7,
            [PG_TOP] = 0, [PG_BOTTOM] = 6 },
        .add_model = tile_model_basic },
};

/*  Public interface    */
void bork_map_init(struct bork_map* map, struct pg_texture* tex,
                   struct pg_shader* shader)
{
    map->tex_atlas = tex;
    map->shader = shader;
    int i;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        map->area_pos[i][0] = 0;
        map->area_pos[i][1] = 0;
        map->area_pos[i][2] = i * 32;
        map->data[i] = calloc(32 * 32 * 32, sizeof(struct bork_tile));
        pg_model_init(&map->area_model[i]);
        int j;
        for(j = 0; j < 32 * 32 * 32 - 32 * 32; ++j) {
            map->data[i][j] = (struct bork_tile){ BORK_TILE_HULL };
        }
        bork_map_generate_area_model(map, i);
        pg_shader_buffer_model(shader, &map->area_model[i]);
        ARR_INIT(map->objects[i]);
        if(i == 0) {
            struct bork_map_object test_door = {
                .type = BORK_MAP_OBJ_DOOR,
                .x = 16, .y = 16, .z = 31 };
            ARR_PUSH(map->objects[i], test_door);
        }
    }
    pg_model_init(&map->door_model);
    vec4 face_uv[6];
    for(i = 0; i < 6; ++i) {
        pg_texture_get_frame(tex, 2, face_uv[i], face_uv[i] + 2);
    }
    pg_model_rect_prism(&map->door_model, (vec3){ 0.5, 0.5, 0.5 }, face_uv);
    pg_model_precalc_ntb(&map->door_model);
    pg_shader_buffer_model(shader, &map->door_model);
}

void bork_map_deinit(struct bork_map* map)
{
    int i;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        free(map->data[i]);
        pg_model_deinit(&map->area_model[i]);
    }
}

void bork_map_draw_area(struct bork_map* map, enum bork_area area)
{
    pg_model_begin(&map->area_model[area], map->shader);
    mat4 model_transform;
    mat4_translate(model_transform,
                   map->area_pos[area][0] * 2.0f,
                   map->area_pos[area][1] * 2.0f,
                   map->area_pos[area][2] * 2.0f);
    pg_model_draw(&map->area_model[area], model_transform);
    struct bork_map_object* obj;
    int i;
    pg_model_begin(&map->door_model, map->shader);
    ARR_FOREACH_PTR(map->objects[area], obj, i) {
        mat4_translate(model_transform,
            (map->area_pos[area][0] + obj->x) * 2.0f + 1.0f,
            (map->area_pos[area][1] + obj->y) * 2.0f + 1.0f,
            (map->area_pos[area][2] + obj->z) * 2.0f + 1.0f + obj->door.pos);
        pg_model_draw(&map->door_model, model_transform);
    }
}
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

#if 0
void bork_map_generate_model(struct bork_map* map, struct pg_model* model,
                             struct pg_texture* tex)
{
    pg_model_reset(model);
    model->components = PG_MODEL_COMPONENT_POSITION |
        PG_MODEL_COMPONENT_NORMAL | PG_MODEL_COMPONENT_TAN_BITAN |
        PG_MODEL_COMPONENT_UV;
    map->model = model;
    map->tex_atlas = tex;
    struct bork_tile* tile;
    int x, y, z;
    for(x = 0; x < map->w; ++x) {
        for(y = 0; y < map->l; ++y) {
            for(z = 0; z < map->h; ++z) {
                tile = &map->data[area][x + y * 32 + z * 32 * 32];
                if(!tile || tile->type < 2) continue;
                struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
                tile->model_tri_idx = model->tris.len;
                tile->num_tris = detail->add_model(map, area, tile, x, y, z);
            }
        }
    }
    pg_model_precalc_ntb(model);
}

static void bork_map_generate_area(struct bork_map* map, enum bork_area area);

void bork_map_generate(struct bork_map* map)
{
    memset(map->data, 0, sizeof(*map->data) * map->w * map->l * map->h);
    int order[BORK_AREA_EXTERIOR] = { 0, 1, 2, 3, 4, 5, 6 };
    struct {
        int w, l, h, d;
    } dims[BORK_AREA_EXTERIOR] = {};
    /*  First we randomly switch up the order of the inner sections */
    int i;
    for(i = BORK_AREA_WAREHOUSE; i < BORK_AREA_SCIENCE_LABS; ++i) {
        if(rand() % 6 == 0) {
            int tmp = order[i];
            order[i] = order[i + 1];
            order[i + 1] = tmp;
        }
    }
    /*  Now go through and generate the overall dimensions of each section  */
    int current_d = 4;
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        struct bork_area_detail* detail = &BORK_AREA_DETAILS[order[i]];
        int diff[2] = { detail->w[1] - detail->w[0],
                        detail->h[1] - detail->h[0] };
        dims[i].w = rand() % diff[0] + detail->w[0];
        if(detail->flags & BORK_AREA_FLAG_SQUARE) dims[i].l = dims[i].w;
        else dims[i].l = rand() % diff[0] + detail->w[0];
        dims[i].h = (rand() % diff[1] + detail->h[0]) * 4;
        dims[i].d = current_d;
        current_d += dims[i].h + (rand() % 3 == 0 ? (rand() % 3 + 2) : 0);
        vec3_set(map->area_dims[i][0],
            16 - (dims[i].w / 2), 16 - (dims[i].l / 2), dims[i].d);
        vec3_set(map->area_dims[i][1],
            16 + (dims[i].w / 2), 16 + (dims[i].l / 2), dims[i].d + dims[i].h);
        printf("Area section %d, id %d:\n"
               "  (%f, %f, %f) -> (%f, %f, %f)\n", i, order[i],
               map->area_dims[i][0][0], map->area_dims[i][0][1], map->area_dims[i][0][2],
               map->area_dims[i][1][0], map->area_dims[i][1][1], map->area_dims[i][1][2]);
    }
    /*  Now go through each section and generate individual rooms for each  */
    for(i = 0; i < BORK_AREA_EXTERIOR; ++i) {
        bork_map_generate_area(map, order[i]);
    }
}

static void bork_map_generate_area(struct bork_map* map, enum bork_area area)
{
    int start_x = map->area_dims[area][0][0];
    int start_y = map->area_dims[area][0][1];
    int start_z = map->area_dims[area][0][2];
    int w = map->area_dims[area][1][0] - start_x;
    int l = map->area_dims[area][1][1] - start_y;
    int h = map->area_dims[area][1][2] - start_z;
    int x, y, z;
    for(x = 0; x < w; ++x) {
        for(y = 0; y < l; ++y) {
            for(z = 0; z < h; ++z) {
                if(x == 0 || x == w - 1 || y == 0 || y == l - 1 || z == 0 || z == h - 1) {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_HULL });
                } else if(x == 4 && y == 1) {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_HULL });
                } else if(x == 4 && y == 2) {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_LADDER });
                } else if((x == 1 || y == 1 || x == 8) && (z == 3)) {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_CATWALK });
                } else if(x == 6 && y == 7 && z == 1) {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_HULL });
                } else {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_ATMO });
                }
            }
        }
    }
}
#endif

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
    uint32_t face_flags = detail->face_flags[dir];
    if(!(face_flags & BORK_TILE_HAS_SURFACE)) return 0; /*  Tile has no face here   */
    /*  Get details for the opposing face   */
    int opp[3] = { x + PG_DIR_VEC[dir][0], y + PG_DIR_VEC[dir][1], z + PG_DIR_VEC[dir][2] };
    struct bork_tile* opp_tile = bork_map_tile_ptr(map, area, opp[0], opp[1], opp[2]);
    struct bork_tile_detail* opp_detail = opp_tile ?
        &BORK_TILE_DETAILS[opp_tile->type] : &BORK_TILE_DETAILS[0];
    uint32_t opp_flags = opp_detail->face_flags[PG_DIR_OPPOSITE[dir]];
    /*  Decide if this face of the tile needs to be generated   */
    if(opp_flags & BORK_TILE_HAS_SURFACE && !(opp_flags & BORK_TILE_SEETHRU_SURFACE)) {
        if(!(face_flags & BORK_TILE_FORCE_SURFACE)
        && !(face_flags & BORK_TILE_FLUSH_SURFACE)) return 0;
    } else if(face_flags & BORK_TILE_NO_SELF_OPPOSITE
            && opp_flags & BORK_TILE_NO_SELF_OPPOSITE) {
        return 0;
    } else {
        if(face_flags & BORK_TILE_FLUSH_SURFACE) return 0;
    }
    struct pg_model* model = &map->area_model[area];
    /*  Calculate the base info for this face   */
    int num_tris = 0;
    vec3 inset_dir;
    vec3_scale(inset_dir, PG_DIR_VEC[dir], detail->face_inset[dir]);
    vec2 tex_frame[2];
    pg_texture_get_frame(map->tex_atlas, detail->tex_tile[dir],
                         tex_frame[0], tex_frame[1]);
    unsigned vert_idx = model->v_count;
    /*  Generate the geometry   */
    struct pg_vertex_full new_vert = { .components =
        PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV };
    int i;
    if(!(face_flags & BORK_TILE_NO_FRONTFACE)) {
        for(i = 0; i < 4; ++i) {
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            vec2_set(new_vert.uv, tex_frame[(i < 2)][0], tex_frame[(i % 2)][1]);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 0, vert_idx + 2);
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 2, vert_idx + 3);
        num_tris += 2;
    }
    if(face_flags & BORK_TILE_HAS_BACKFACE) {
        for(i = 0; i < 4; ++i) {
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            vec2_set(new_vert.uv, tex_frame[(i < 2)][0], tex_frame[(i % 2)][1]);
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


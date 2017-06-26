#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "map_area.h"

void bork_map_init(struct bork_map* map, int w, int l, int h)
{
    *map = (struct bork_map) {
        .data = calloc(w * l * h, sizeof(struct bork_tile)),
        .w = w,
        .l = l,
        .h = h };
}

void bork_map_deinit(struct bork_map* map)
{
    free(map->data);
}

static const uint32_t comp = PG_MODEL_COMPONENT_POSITION |
    PG_MODEL_COMPONENT_NORMAL | PG_MODEL_COMPONENT_TAN_BITAN |
    PG_MODEL_COMPONENT_UV;

static const vec3 vert_norm[6] = {
    [BORK_FRONT] = { 0, 1.0, 0 }, 
    [BORK_BACK] = { 0, -1.0, 0 },
    [BORK_LEFT] = { 1.0, 0, 0 }, 
    [BORK_RIGHT] = { -1.0, 0, 0 }, 
    [BORK_UP] = { 0, 0, 1.0 }, 
    [BORK_DOWN] = { 0, 0, -1.0 } };
static const vec3 vert_tan[6] = {
    [BORK_FRONT] = { -1.0, 0, 0 }, 
    [BORK_BACK] = { 1.0, 0, 0 },
    [BORK_LEFT] = { 0, 1.0, 0 }, 
    [BORK_RIGHT] = { 0, -1.0, 0 }, 
    [BORK_UP] = { 1.0, 0, 0 }, 
    [BORK_DOWN] = { 1.0, 0, 0 } };
static const vec3 vert_bitan[6] = {
    [BORK_FRONT] = { 0, 0, -1.0 }, 
    [BORK_BACK] = { 0, 0, -1.0 },
    [BORK_LEFT] = { 0, 0, -1.0 }, 
    [BORK_RIGHT] = { 0, 0, -1.0 }, 
    [BORK_UP] = { 0, -1.0, 0 }, 
    [BORK_DOWN] = { 0, 1.0, 0 } };
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
static const int vert_uv[4][2] = { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };

static int bork_map_model_add_face(struct bork_map* map, struct bork_tile* tile,
                                   int x, int y, int z,
                                   enum bork_direction dir)
{
    struct pg_vertex_full new_vert = { .components = comp };
    struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
    uint32_t face_flags = detail->face_flags[dir];
    if(!(face_flags & BORK_TILE_HAS_SURFACE)) return 0;
    int opp[3] = { x + BORK_DIR[dir][0], y + BORK_DIR[dir][1], z + BORK_DIR[dir][2] };
    struct bork_tile* opp_tile = bork_map_get_tile(map, opp[0], opp[1], opp[2]);
    struct bork_tile_detail* opp_detail = opp_tile ? 
        &BORK_TILE_DETAILS[opp_tile->type] : &BORK_TILE_DETAILS[0];
    enum bork_direction opp_dir = BORK_DIR_OPPOSITE[dir];
    uint32_t opp_flags = opp_detail->face_flags[opp_dir];
    /*  Decide if this face of the tile needs to be generated   */
    if(opp_flags & BORK_TILE_HAS_SURFACE && !(opp_flags & BORK_TILE_SEETHRU_SURFACE)) {
        if(!(face_flags & BORK_TILE_FORCE_SURFACE)
        && !(face_flags & BORK_TILE_FLUSH_SURFACE)) return 0;
    } else {
        if(face_flags & BORK_TILE_FLUSH_SURFACE) return 0;
    }
    unsigned vert_idx = map->model->v_count;
    int i;
    if(!(face_flags & BORK_TILE_NO_FRONTFACE)) {
        for(i = 0; i < 4; ++i) {
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3 inset_dir;
            vec3_scale(inset_dir, vert_norm[dir], detail->face_inset[dir]);
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            vec3_dup(new_vert.normal, vert_norm[dir]);
            vec3_dup(new_vert.tangent, vert_tan[dir]);
            vec3_dup(new_vert.bitangent, vert_bitan[dir]);
            vec2 tex_frame[2];
            pg_texture_get_frame(map->tex_atlas, detail->tex_tile[dir],
                                 tex_frame[0], tex_frame[1]);
            vec2_set(new_vert.uv, tex_frame[vert_uv[i][0]][0], tex_frame[vert_uv[i][1]][1]);
            pg_model_add_vertex(map->model, &new_vert);
        }
    }
    if(face_flags & BORK_TILE_HAS_BACKFACE) {
        for(i = 0; i < 4; ++i) {
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3 inset_dir;
            vec3_scale(inset_dir, vert_norm[dir], detail->face_inset[dir]);
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            vec3_dup(new_vert.normal, vert_norm[BORK_DIR_OPPOSITE[dir]]);
            vec3_dup(new_vert.tangent, vert_tan[dir]);
            vec3_dup(new_vert.bitangent, vert_bitan[dir]);
            vec2 tex_frame[2];
            pg_texture_get_frame(map->tex_atlas, detail->tex_tile[dir],
                                 tex_frame[0], tex_frame[1]);
            vec2_set(new_vert.uv, tex_frame[vert_uv[i][0]][0], tex_frame[vert_uv[i][1]][1]);
            pg_model_add_vertex(map->model, &new_vert);
        }
    }
    int num_tris = 0;
    if(!(face_flags & BORK_TILE_NO_FRONTFACE)) {
        pg_model_add_triangle(map->model, vert_idx + 1, vert_idx + 0, vert_idx + 2);
        pg_model_add_triangle(map->model, vert_idx + 1, vert_idx + 2, vert_idx + 3);
        num_tris += 2;
    }
    if(face_flags & BORK_TILE_HAS_BACKFACE) {
        pg_model_add_triangle(map->model, vert_idx + 4, vert_idx + 5, vert_idx + 6);
        pg_model_add_triangle(map->model, vert_idx + 6, vert_idx + 5, vert_idx + 7);
        num_tris += 2;
    }
    return num_tris;
}

void bork_map_generate_model(struct bork_map* map, struct pg_model* model,
                             struct pg_texture* tex)
{
    pg_model_reset(model);
    model->components = comp;
    map->model = model;
    struct bork_tile* tile;
    int x, y, z;
    for(x = 0; x < map->w; ++x) {
        for(y = 0; y < map->l; ++y) {
            for(z = 0; z < map->h; ++z) {
                tile = bork_map_get_tile(map, x, y, z);
                if(!tile || tile->type < 2) continue;
                tile->num_tris = 0;
                tile->model_tri_idx = model->tris.len;
                int s;
                for(s = 0; s < 6; ++s) {
                    tile->num_tris += bork_map_model_add_face(map, tile, x, y, z, s);
                }
            }
        }
    }
    pg_model_warp_verts(model);
}

struct bork_tile* bork_map_get_tile(struct bork_map* map, int x, int y, int z)
{
    if(x < 0 || x >= map->w || y < 0 || y >= map->l || z < 0 || z >= map->h)
        return NULL;
    return &map->data[x + y * map->w + z * map->w * map->l];
}

void bork_map_set_tile(struct bork_map* map, int x, int y, int z,
                       struct bork_tile tile)
{
    if(x < 0 || x >= map->w || y < 0 || y >= map->l || z < 0 || z >= map->h)
        return;
    map->data[x + y * map->w + z * map->w * map->l] = tile;
}

static void bork_map_generate_area(struct bork_map* map, enum bork_area area);

void bork_map_generate(struct bork_map* map)
{
    memset(map->data, 0, sizeof(*map->data) * map->w * map->l * map->h);
    #if 1
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
    #endif
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
                } else {
                    bork_map_set_tile(map, x + start_x, y + start_y, z + start_z,
                        (struct bork_tile){ .type = BORK_TILE_ATMO });
                }
            }
        }
    }
}

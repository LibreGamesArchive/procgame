#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "map_area.h"

void bork_map_init(struct bork_map* map, int w, int l, int h)
{
    map->data = calloc(w * l * h, sizeof(struct bork_tile));
    map->w = w;
    map->l = l;
    map->h = h;
    map->tex_atlas = NULL;
}

void bork_map_deinit(struct bork_map* map)
{
    free(map->data);
}

static const uint32_t comp = PG_MODEL_COMPONENT_POSITION |
    PG_MODEL_COMPONENT_NORMAL | PG_MODEL_COMPONENT_TAN_BITAN |
    PG_MODEL_COMPONENT_UV;

enum face_dir {
    FACE_FRONT, FACE_BACK, FACE_LEFT, FACE_RIGHT, FACE_UP, FACE_DOWN };

static const vec3 vert_norm[6] = {
    [FACE_FRONT] = { 0, 1.0, 0 }, 
    [FACE_BACK] = { 0, -1.0, 0 },
    [FACE_LEFT] = { 1.0, 0, 0 }, 
    [FACE_RIGHT] = { -1.0, 0, 0 }, 
    [FACE_UP] = { 0, 0, 1.0 }, 
    [FACE_DOWN] = { 0, 0, -1.0 } };
static const vec3 vert_tan[6] = {
    [FACE_FRONT] = { -1.0, 0, 0 }, 
    [FACE_BACK] = { 1.0, 0, 0 },
    [FACE_LEFT] = { 0, 1.0, 0 }, 
    [FACE_RIGHT] = { 0, -1.0, 0 }, 
    [FACE_UP] = { 1.0, 0, 0 }, 
    [FACE_DOWN] = { 1.0, 0, 0 } };
static const vec3 vert_bitan[6] = {
    [FACE_FRONT] = { 0, 0, -1.0 }, 
    [FACE_BACK] = { 0, 0, -1.0 },
    [FACE_LEFT] = { 0, 0, -1.0 }, 
    [FACE_RIGHT] = { 0, 0, -1.0 }, 
    [FACE_UP] = { 0, -1.0, 0 }, 
    [FACE_DOWN] = { 0, 1.0, 0 } };
static const vec3 vert_pos[4] = {
    { -0.5, -0.5, -0.5 },
    { -0.5, -0.5, 0.5 },
    { 0.5, -0.5, -0.5 },
    { 0.5, -0.5, 0.5 } };
static const int vert_uv[4][2] = { { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 1 } };

static void bork_map_model_add_face(struct pg_model* model,
                                    struct pg_texture* tex_atlas,
                                    enum bork_tile_type tile_type,
                                    enum face_dir dir, int x, int y, int z)
{
    struct pg_vertex_full new_vert = { .components = comp };
    int i;
    for(i = 0; i < 4; ++i) {
        switch(dir) {
        case FACE_FRONT:
            vec3_set(new_vert.pos, -vert_pos[i][0] + (float)x,
                -vert_pos[i][1] + (float)y, vert_pos[i][2] + (float)z );
            break;
        case FACE_BACK:
            vec3_set(new_vert.pos, vert_pos[i][0] + (float)x,
                vert_pos[i][1] + (float)y, vert_pos[i][2] + (float)z );
            break;
        case FACE_LEFT:
            vec3_set(new_vert.pos, -vert_pos[i][1] + (float)x,
                vert_pos[i][0] + (float)y, vert_pos[i][2] + (float)z );
            break;
        case FACE_RIGHT:
            vec3_set(new_vert.pos, vert_pos[i][1] + (float)x,
                -vert_pos[i][0] + (float)y, vert_pos[i][2] + (float)z );
            break;
        case FACE_UP:
            vec3_set(new_vert.pos, vert_pos[i][0] + (float)x,
                vert_pos[i][2] + (float)y, -vert_pos[i][1] + (float)z );
            break;
        case FACE_DOWN:
            vec3_set(new_vert.pos, vert_pos[i][0] + (float)x,
                -vert_pos[i][2] + (float)y, vert_pos[i][1] + (float)z );
            break;
        }
        vec3_scale(new_vert.pos, new_vert.pos, 2);
        vec3_dup(new_vert.normal, vert_norm[dir]);
        vec3_dup(new_vert.tangent, vert_tan[dir]);
        vec3_dup(new_vert.bitangent, vert_bitan[dir]);
        vec2 tex_frame[2];
        pg_texture_get_frame(tex_atlas, tile_type, tex_frame[0], tex_frame[1]);
        vec2_set(new_vert.uv, tex_frame[vert_uv[i][0]][0], tex_frame[vert_uv[i][1]][1]);
        pg_model_add_vertex(model, &new_vert);
    }
    unsigned vert_idx = model->v_count - 4;
    pg_model_add_triangle(model, vert_idx + 1, vert_idx + 0, vert_idx + 2);
    pg_model_add_triangle(model, vert_idx + 1, vert_idx + 2, vert_idx + 3);
}

void bork_map_generate_model(struct bork_map* map, struct pg_model* model,
                             struct pg_texture* tex)
{
    pg_model_reset(model);
    model->components = comp;
    struct bork_tile* tile;
    int x, y, z;
    for(x = 0; x < map->w; ++x) {
        for(y = 0; y < map->l; ++y) {
            for(z = 0; z < map->h; ++z) {
                tile = bork_map_get_tile(map, x, y, z);
                if(!tile || tile->type < 2) continue;
                struct bork_tile* surr_tiles[6] = {
                    bork_map_get_tile(map, x, y + 1, z),    /* Front */
                    bork_map_get_tile(map, x, y - 1, z),    /* Back */
                    bork_map_get_tile(map, x + 1, y, z),    /* Left */
                    bork_map_get_tile(map, x - 1, y, z),    /* Right */
                    bork_map_get_tile(map, x, y, z + 1),    /* Up */
                    bork_map_get_tile(map, x, y, z - 1) };  /* Down */
                int s;
                for(s = 0; s < 6; ++s) {
                    if(surr_tiles[s] && surr_tiles[s]->type > 1) continue;
                    bork_map_model_add_face(model, tex, tile->type,
                                            s, x, y, z);
                }
            }
        }
    }

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

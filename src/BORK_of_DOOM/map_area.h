enum bork_area {
    BORK_AREA_EXTERIOR,
    BORK_AREA_MUTT,
    BORK_AREA_COMMAND,
    BORK_AREA_WAREHOUSE,
    BORK_AREA_UNION_HALL,
    BORK_AREA_INFIRMARY,
    BORK_AREA_REC_CENTER,
    BORK_AREA_SCIENCE_LABS,
    BORK_AREA_CAFETERIA,
    BORK_AREA_KITCHEN,
    BORK_AREA_PETS
};

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL
};

static struct bork_tile_detail {
    enum {
        BORK_TILE_EMPTY,
        BORK_TILE_REGULAR,
        BORK_TILE_MODEL } type;
    uint8_t face_flags[6];
    int tex_tile[6];
    struct pg_model model;
} BORK_TILE_DETAILS[] = {
    [BORK_TILE_VAC] = {},
    [BORK_TILE_ATMO] = {},
    [BORK_TILE_HULL] = {
        BORK_TILE_REGULAR,
        .tex_tile = {
            [BORK_LEFT] = 2, [BORK_RIGHT] = 2,
            [BORK_FRONT] = 2, [BORK_BACK] = 2,
            [BORK_UP] = 3, [BORK_DOWN] = 4 } }
};

struct bork_tile {
    enum bork_tile_type type;
    uint16_t flags;
    uint16_t num_tris;
    uint32_t model_tri_idx;
};

struct bork_map {
    box station_areas[BORK_AREA_PETS];
    int w, l, h;
    struct bork_tile* data;
    struct pg_texture* tex_atlas;
    struct pg_model* model;
};

void bork_map_init(struct bork_map* map, int w, int l, int h);
void bork_map_deinit(struct bork_map* map);
void bork_map_generate_model(struct bork_map* map, struct pg_model* model,
                             struct pg_texture* tex);
struct bork_tile* bork_map_get_tile(struct bork_map* map, int x, int y, int z);
void bork_map_set_tile(struct bork_map* map, int x, int y, int z,
                       struct bork_tile tile);
int bork_map_collide(struct bork_map* map, vec3 out, vec3 pos, vec3 size);

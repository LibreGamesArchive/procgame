enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL
};

struct bork_tile {
    enum bork_tile_type type;
    uint32_t flags;
    uint32_t model_tri_idx;
};

struct bork_map {
    int w, l, h;
    struct bork_tile* data;
    struct pg_texture* tex_atlas;
};

void bork_map_init(struct bork_map* map, int w, int l, int h);
void bork_map_deinit(struct bork_map* map);
void bork_map_generate_model(struct bork_map* map, struct pg_model* model,
                             struct pg_texture* tex);
struct bork_tile* bork_map_get_tile(struct bork_map* map, int x, int y, int z);
void bork_map_set_tile(struct bork_map* map, int x, int y, int z,
                       struct bork_tile tile);

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL
};

struct bork_tile {
    enum bork_tile_type type;
    uint16_t flags;
    uint16_t num_tris;
    uint32_t model_tri_idx;
};

struct bork_map {
    int station_area[2]; /* z start, z end  */
    int area_radius[10];
    int area_order[10];
    int mutt_area[5]; /* x, y, z start, z end, radius */
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

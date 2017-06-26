enum bork_area {
    BORK_AREA_PETS,
    BORK_AREA_WAREHOUSE,
    BORK_AREA_CAFETERIA,
    BORK_AREA_REC_CENTER,
    BORK_AREA_INFIRMARY,
    BORK_AREA_SCIENCE_LABS,
    BORK_AREA_COMMAND,
    BORK_AREA_EXTERIOR,
    BORK_AREA_MUTT,
};

#define BORK_AREA_FLAG_SQUARE   (1 << 0)

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

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL,
    BORK_TILE_LADDER
};

#define BORK_TILE_HAS_SURFACE   (1 << 0)
#define BORK_TILE_SEETHRU_SURFACE  (1 << 1)
#define BORK_TILE_FORCE_SURFACE (1 << 2)
#define BORK_TILE_FLUSH_SURFACE (1 << 3)
#define BORK_TILE_HAS_BACKFACE  (1 << 4)
#define BORK_TILE_NO_FRONTFACE  (1 << 5)

static struct bork_tile_detail {
    uint8_t face_flags[6];
    float face_inset[6];
    int tex_tile[6];
    struct pg_model model;
} BORK_TILE_DETAILS[] = {
    [BORK_TILE_VAC] = {},
    [BORK_TILE_ATMO] = {},
    [BORK_TILE_HULL] = {
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [BORK_LEFT] = 2, [BORK_RIGHT] = 2,
            [BORK_FRONT] = 2, [BORK_BACK] = 2,
            [BORK_UP] = 3, [BORK_DOWN] = 4 }
    },
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
            [BORK_LEFT] = 5, [BORK_RIGHT] = 5,
            [BORK_FRONT] = 5, [BORK_BACK] = 5 }
    }
};

struct bork_tile {
    enum bork_tile_type type;
    uint16_t flags;
    uint16_t num_tris;
    uint32_t model_tri_idx;
};

struct bork_map {
    int station_order[BORK_AREA_EXTERIOR];
    box area_dims[BORK_AREA_EXTERIOR];
    int w, l, h;
    struct bork_tile* data;
    struct pg_texture* tex_atlas;
    struct pg_model* model;
};

void bork_map_init(struct bork_map* map, int w, int l, int h);
void bork_map_deinit(struct bork_map* map);
void bork_map_generate(struct bork_map* map);
void bork_map_generate_model(struct bork_map* map, struct pg_model* model,
                             struct pg_texture* tex);
struct bork_tile* bork_map_get_tile(struct bork_map* map, int x, int y, int z);
void bork_map_set_tile(struct bork_map* map, int x, int y, int z,
                       struct bork_tile tile);

/*  See physics.c   */

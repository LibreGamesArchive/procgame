#include "map_object.h"

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

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL,
    BORK_TILE_LADDER,
    BORK_TILE_CATWALK,
};

#define BORK_TILE_HAS_SURFACE       (1 << 0)
#define BORK_TILE_SEETHRU_SURFACE   (1 << 1)
#define BORK_TILE_FORCE_SURFACE     (1 << 2)
#define BORK_TILE_FLUSH_SURFACE     (1 << 3)
#define BORK_TILE_NO_SELF_OPPOSITE  (1 << 4)
#define BORK_TILE_HAS_BACKFACE      (1 << 5)
#define BORK_TILE_NO_FRONTFACE      (1 << 6)
#define BORK_TILE_SPECIAL_MODEL     (1 << 7)

struct bork_tile {
    enum bork_tile_type type;
    uint8_t flags;
    uint8_t orientation;
    uint16_t num_tris;
    uint32_t model_tri_idx;
};

struct bork_map {
    int area_pos[BORK_AREA_EXTERIOR][3];
    struct bork_tile* data[BORK_AREA_EXTERIOR];
    struct pg_model area_model[BORK_AREA_EXTERIOR];
    ARR_T(struct bork_map_object) objects[BORK_AREA_EXTERIOR];
    struct pg_texture* tex_atlas;
    struct pg_shader* shader;
    struct pg_model door_model;
};

struct bork_tile_detail {
    uint32_t face_flags[6];
    float face_inset[6];
    int tex_tile[6];
    int (*add_model)(struct bork_map*, enum bork_area,
                     struct bork_tile*, int, int, int);
};

void bork_map_init(struct bork_map* map, struct pg_texture* tex,
                   struct pg_shader* shader);
void bork_map_deinit(struct bork_map* map);
void bork_map_draw_area(struct bork_map* map, enum bork_area area);
enum bork_area bork_map_get_area(struct bork_map* map, int x, int y, int z);
struct bork_tile* bork_map_tile_ptr(struct bork_map* map, enum bork_area area,
                                    int x, int y, int z);

/*  See physics.c   */

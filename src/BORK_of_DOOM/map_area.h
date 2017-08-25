enum bork_area {
    BORK_AREA_PETS,
    BORK_AREA_WAREHOUSE,
    BORK_AREA_CAFETERIA,
    BORK_AREA_REC_CENTER,
    BORK_AREA_INFIRMARY,
    BORK_AREA_SCIENCE_LABS,
    BORK_AREA_COMMAND,
    BORK_AREA_MUTT,
    BORK_AREA_EXTERIOR,
};
const char* bork_map_area_str(enum bork_area area);

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL,
    BORK_TILE_HULL_HALF,
    BORK_TILE_HULL_EDGE,
    BORK_TILE_CONTROL_PANEL,
    BORK_TILE_LADDER,
    BORK_TILE_CATWALK,
    BORK_TILE_CATWALK_HALF,
    BORK_TILE_HANDRAIL,
    BORK_TILE_HANDRAIL_TOP,
    BORK_TILE_POWERBLOCK,
    BORK_TILE_TABLE,
    BORK_TILE_GARDEN,
    BORK_TILE_RAMP_BOTTOM, BORK_TILE_RAMP_TOP,
    BORK_TILE_EDITOR_DOOR,
    BORK_TILE_EDITOR_LIGHT1,
    BORK_TILE_EDITOR_LIGHT_WALLMOUNT,
    BORK_TILE_COUNT,
};

#define BORK_FACE_HAS_SURFACE       (1 << 0)
#define BORK_FACE_SEETHRU_SURFACE   (1 << 1)
#define BORK_FACE_FORCE_SURFACE     (1 << 2)
#define BORK_FACE_FLUSH_SURFACE     (1 << 3)
#define BORK_FACE_NO_SELF_OPPOSITE  (1 << 4)
#define BORK_FACE_HAS_BACKFACE      (1 << 5)
#define BORK_FACE_NO_FRONTFACE      (1 << 6)
#define BORK_FACE_HAS_ORIENTATION   (1 << 8)
#define BORK_FACE_HALF_BOTTOM       (1 << 9)
#define BORK_FACE_HALF_TOP          (1 << 10)

#define BORK_TILE_SPECIAL_MODEL     (1 << 0)
#define BORK_TILE_HAS_ORIENTATION   (1 << 1)
#define BORK_TILE_FACE_ORIENTED     (1 << 2)

struct bork_tile {
    enum bork_tile_type type;
    uint8_t flags;
    uint8_t orientation;
    uint16_t num_tris;
    uint32_t model_tri_idx;
};

struct bork_map_door {
    int x, y, z;
    int dir;
    int locked;
    float pos;
};

struct bork_map_light_fixture {
    vec3 pos;
    int type;
};

struct bork_map {
    struct pg_model model;
    struct bork_tile data[32][32][32];
    ARR_T(struct bork_map_door) doors;
    ARR_T(struct bork_map_light_fixture) light_fixtures;
    ARR_T(bork_entity_t) enemies;
    ARR_T(bork_entity_t) items;
    ARR_T(struct pg_light) lights;
    ARR_T(struct pg_light) spotlights;
    struct pg_model door_model;
    struct bork_entity* plr;
};

struct bork_tile_detail {
    uint32_t tile_flags;
    uint32_t face_flags[6];
    float face_inset[6];
    int tex_tile[6];
    char name[32];
    int (*add_model)(struct bork_map*, struct pg_texture*,
                     struct bork_tile*, int, int, int);
};
const struct bork_tile_detail* bork_tile_detail(enum bork_tile_type type);

void bork_map_init(struct bork_map* map);
void bork_map_init_model(struct bork_map* map, struct bork_game_core* core);
void bork_map_deinit(struct bork_map* map);
struct bork_tile* bork_map_tile_ptr(struct bork_map* map, vec3 const pos);
struct bork_tile* bork_map_tile_ptri(struct bork_map* map, int x, int y, int z);
void bork_map_write_to_file(struct bork_map* map, char* filename);
void bork_map_load_from_file(struct bork_map* map, char* filename);
void bork_map_update(struct bork_map* map, struct bork_entity* plr);
void bork_map_draw(struct bork_map* map, struct bork_game_core* core);
int bork_map_check_sphere(struct bork_map* map, vec3 const pos, float r);
int bork_map_check_vis(struct bork_map* map, vec3 const start, vec3 const end);

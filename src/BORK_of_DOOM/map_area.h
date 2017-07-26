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

static const char* BORK_AREA_STRING[] = {
    [BORK_AREA_PETS] = "P.E.T.S.",
    [BORK_AREA_WAREHOUSE] = "WAREHOUSE",
    [BORK_AREA_CAFETERIA] = "CAFETERIA",
    [BORK_AREA_REC_CENTER] = "DOG PARK",
    [BORK_AREA_INFIRMARY] = "VETERINARY CLINIC",
    [BORK_AREA_SCIENCE_LABS] = "SCIENCE LABS",
    [BORK_AREA_COMMAND] = "COMMAND CENTER",
    [BORK_AREA_EXTERIOR] = "BONZ EXTERIOR",
    [BORK_AREA_MUTT] = "M.U.T.T."
};

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL,
    BORK_TILE_LADDER,
    BORK_TILE_CATWALK,
    BORK_TILE_HANDRAIL,
    BORK_TILE_EDITOR_DOOR,
    BORK_TILE_EDITOR_LIGHT1,
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

struct bork_map_object {
    int x, y, z;
    enum {
        BORK_MAP_OBJ_DOOR,
        BORK_MAP_OBJ_LIGHT,
    } type;
    union {
        struct {
            int dir;
            int locked;
            float pos;
        } door;
        vec4 light;
    };
};

struct bork_map {
    int area_pos[BORK_AREA_EXTERIOR][3];
    struct bork_tile* data[BORK_AREA_EXTERIOR];
    struct pg_model area_model[BORK_AREA_EXTERIOR];
    ARR_T(struct bork_map_object) objects[BORK_AREA_EXTERIOR];
    ARR_T(struct bork_entity) enemies[BORK_AREA_EXTERIOR];
    struct pg_model door_model;
    struct bork_game_core* core;
    struct bork_entity* plr;
};

struct bork_tile_detail {
    uint32_t tile_flags;
    uint32_t face_flags[6];
    float face_inset[6];
    int tex_tile[6];
    char name[32];
    int (*add_model)(struct bork_map*, enum bork_area,
                     struct bork_tile*, int, int, int);
};

void bork_map_init(struct bork_map* map, struct bork_game_core* core);
void bork_map_init_model(struct bork_map* map);
void bork_map_deinit(struct bork_map* map);
void bork_map_write_to_file(struct bork_map* map, char* filename);
void bork_map_load_from_file(struct bork_map* map, char* filename);
void bork_map_update_area(struct bork_map* map, enum bork_area area,
                          struct bork_entity* plr);
void bork_map_draw_area(struct bork_map* map, enum bork_area area);
enum bork_area bork_map_get_area(struct bork_map* map, int x, int y, int z);
struct bork_tile* bork_map_tile_ptr(struct bork_map* map, enum bork_area area,
                                    int x, int y, int z);
const struct bork_tile_detail* bork_tile_detail(enum bork_tile_type type);
/*  See physics.c   */

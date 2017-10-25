struct bork_editor_map;

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

#define BORK_FACE_HAS_SURFACE       (1 << 0)
#define BORK_FACE_SEETHRU_SURFACE   (1 << 1)
#define BORK_FACE_FORCE_SURFACE     (1 << 2)
#define BORK_FACE_FLUSH_SURFACE     (1 << 3)
#define BORK_FACE_NO_SELF_OPPOSITE  (1 << 4)
#define BORK_FACE_HAS_BACKFACE      (1 << 5)
#define BORK_FACE_NO_FRONTFACE      (1 << 6)
#define BORK_FACE_HALF_BOTTOM       (1 << 7)
#define BORK_FACE_HALF_TOP          (1 << 8)
#define BORK_FACE_TRAVEL            (1 << 9)
#define BORK_FACE_TRAVEL_SELF_ONLY  (1 << 10)
#define BORK_FACE_TRAVEL_ORIENT     (1 << 11)
#define BORK_FACE_TRAVEL_ORIENT_OPP (1 << 12)

#define BORK_TILE_SPECIAL_MODEL     (1 << 0)
#define BORK_TILE_HAS_ORIENTATION   (1 << 1)
#define BORK_TILE_FACE_ORIENTED     (1 << 2)
#define BORK_TILE_WALK_ABOVE        (1 << 3)
#define BORK_TILE_TRAVEL_DROP       (1 << 4)

enum bork_tile_type {
    BORK_TILE_VAC,
    BORK_TILE_ATMO,
    BORK_TILE_HULL,
    BORK_TILE_HULL_HALF,
    BORK_TILE_HULL_EDGE,
    BORK_TILE_HULL_WHITE_CAUTION,
    BORK_TILE_HULL_WHITE_LIGHT,
    BORK_TILE_HULL_PANELS,
    BORK_TILE_OVEN,
    BORK_TILE_BED,
    BORK_TILE_CONTROL_PANEL,
    BORK_TILE_LADDER,
    BORK_TILE_CATWALK,
    BORK_TILE_CATWALK_HALF,
    BORK_TILE_HANDRAIL,
    BORK_TILE_HANDRAIL_TOP,
    BORK_TILE_POWERBLOCK,
    BORK_TILE_TABLE,
    BORK_TILE_TABLE_SMALL,
    BORK_TILE_GARDEN,
    BORK_TILE_CARGO_RED, BORK_TILE_CARGO_BLUE,
    BORK_TILE_DUCT,
    BORK_TILE_RAMP_BOTTOM, BORK_TILE_RAMP_TOP,
    BORK_TILE_EDITOR_DOOR,
    BORK_TILE_EDITOR_RECYCLER,
    BORK_TILE_EDITOR_TEXT,
    BORK_TILE_EDITOR_LIGHT1,
    BORK_TILE_EDITOR_LIGHT_WALLMOUNT,
    BORK_TILE_EDITOR_LIGHT_SMALLMOUNT,
    BORK_TILE_EDITOR_LIGHT2,
    BORK_TILE_COUNT,
};

struct bork_tile {
    uint8_t type;
    uint8_t travel_flags;
    uint8_t orientation;
    uint8_t num_tris;
    uint32_t model_tri_idx;
};

struct bork_map_object {
    enum bork_map_object_type {
        BORK_MAP_DOOR,
        BORK_MAP_DOORPAD,
        BORK_MAP_RECYCLER,
        BORK_MAP_TEXT,
        BORK_MAP_GRATE
    } type;
    int dead;
    vec3 pos;
    quat dir;
    union {
        struct {
            int open;
            int locked;
            uint8_t code[4];
            float pos;
        } door;
        struct {
            int door_idx;
        } doorpad;
        struct {
            vec3 out_pos;
        } recycler;
        struct {
            char text[16];
            vec4 color;
            float scale;
        } text;
    };
};

struct bork_map_light_fixture {
    vec3 pos;
    int type;
};

#define BORK_FIRE_MOVES     (1 << 0)

struct bork_fire {
    int lifetime;
    uint32_t flags;
    vec3 pos;
    vec3 vel;
};

struct bork_map {
    struct pg_model model;
    struct bork_tile data[32][32][32];
    uint8_t plr_dist[32][32][32];
    bork_entity_arr_t enemies[4][4][4];
    bork_entity_arr_t entities[4][4][4];
    bork_entity_arr_t items[4][4][4];
    ARR_T(struct bork_map_object) doors;
    ARR_T(struct bork_map_object) doorpads;
    ARR_T(struct bork_map_object) recyclers;
    ARR_T(struct bork_map_object) texts;
    ARR_T(struct bork_map_object) grates;
    ARR_T(struct bork_map_light_fixture) light_fixtures;
    ARR_T(struct bork_fire) fires;
    ARR_T(struct pg_light) lights;
    ARR_T(struct pg_light) spotlights;
    struct pg_model door_model;
    struct pg_model recycler_model;
    struct pg_model oven_model;
    struct pg_model bed_model;
    struct pg_model small_table_model;
    struct pg_model grate_model;
    struct bork_entity* plr;
};

struct bork_tile_detail {
    uint32_t tile_flags;
    uint32_t face_flags[6];
    float face_inset[6];
    int tex_tile[6];
    char name[32];
    int (*add_model)(struct bork_map*, struct bork_editor_map*, struct pg_texture*,
                     struct bork_tile*, int, int, int);
};
const struct bork_tile_detail* bork_tile_detail(enum bork_tile_type type);

void bork_map_init(struct bork_map* map);
void bork_map_reset(struct bork_map* map);
void bork_map_init_model(struct bork_map* map, struct bork_editor_map* ed_map,
                         struct bork_game_core* core);
void bork_map_deinit(struct bork_map* map);
struct bork_tile* bork_map_tile_ptr(struct bork_map* map, vec3 const pos);
struct bork_tile* bork_map_tile_ptri(struct bork_map* map, int x, int y, int z);
void bork_map_write_to_file(struct bork_map* map, char* filename);
void bork_map_load_from_file(struct bork_map* map, char* filename, int newgame);
void bork_map_update(struct bork_map* map, struct bork_entity* plr);
void bork_map_draw(struct bork_map* map, struct bork_game_core* core);
int bork_map_check_ellipsoid(struct bork_map* map, vec3 const pos, vec3 const r);
int bork_map_check_sphere(struct bork_map* map, struct bork_map_object** hit_obj,
                          vec3 const pos, float r);
int bork_map_check_vis(struct bork_map* map, vec3 const start, vec3 const end);
float bork_map_vis_dist(struct bork_map* map, vec3 const start, vec3 const dir);
int bork_map_tile_walkable(struct bork_map* map, int x, int y, int z);
void bork_map_build_plr_dist(struct bork_map* map, vec3 pos);
void bork_map_calc_travel(struct bork_map* map);

void bork_map_create_fire(struct bork_map* map, vec3 pos, int lifetime);
void bork_map_add_enemy(struct bork_map* map, bork_entity_t ent_id);
void bork_map_add_item(struct bork_map* map, bork_entity_t ent_id);
void bork_map_add_entity(struct bork_map* map, bork_entity_t ent_id);
void bork_map_query_enemies(struct bork_map* map, bork_entity_arr_t* arr,
                            vec3 start, vec3 end);
void bork_map_query_items(struct bork_map* map, bork_entity_arr_t* arr,
                          vec3 start, vec3 end);
void bork_map_query_entities(struct bork_map* map, bork_entity_arr_t* arr,
                             vec3 start, vec3 end);



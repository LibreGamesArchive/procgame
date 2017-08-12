struct bork_map;
struct bork_play_data;

#define BORK_ENTFLAG_DEAD       (1 << 0)
#define BORK_ENTFLAG_INACTIVE   (1 << 1)
#define BORK_ENTFLAG_GROUND     (1 << 2)
#define BORK_ENTFLAG_LOOKED_AT  (1 << 3)
#define BORK_ENTFLAG_ITEM       (1 << 4)

struct bork_entity {
    vec3 pos;
    vec3 vel;
    vec2 dir;
    uint32_t flags;
    int still_ticks;
    int HP;
    int counter[4];
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENTITY_ENEMY,
        BORK_ITEM_DOGFOOD,
        BORK_ITEM_BULLETS,
        BORK_ITEM_SCRAPMETAL,
        BORK_ITEM_MACHINEGUN,
        BORK_ENTITY_TYPES,
    } type;
};

void bork_use_machinegun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_machinegun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_dogfood(struct bork_entity* ent, struct bork_play_data* d);

static const struct bork_entity_profile {
    char name[32];
    uint32_t base_flags;
    vec3 size;
    vec4 sprite_tx;
    int front_frame;
    int dir_frames;
    uint8_t use_ctrl;
    void (*use_func)(struct bork_entity*, struct bork_play_data*);
    void (*hud_func)(struct bork_entity*, struct bork_play_data*);
} BORK_ENT_PROFILES[] = {
    [BORK_ENTITY_PLAYER] = { .name = "Player",
        .size = { 0.5, 0.5, 0.9 },
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ENTITY_ENEMY] = { .name = "Enemy",
        .size = { 1, 1, 1 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 192,
        .dir_frames = 4 },
    [BORK_ITEM_DOGFOOD] = { .name = "DOG FOOD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 0,
        .use_ctrl = BORK_CONTROL_HIT,
        .use_func = NULL },
    [BORK_ITEM_MACHINEGUN] = { .name = "MACHINE GUN",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 8,
        .use_ctrl = BORK_CONTROL_HELD,
        .use_func = bork_use_machinegun,
        .hud_func = bork_hud_machinegun },
    [BORK_ITEM_BULLETS] = { .name = "BULLETS",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 1 },
    [BORK_ITEM_SCRAPMETAL] = { .name = "SCRAP METAL",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 2 },
};

typedef int bork_entity_t;
bork_entity_t bork_entity_new(int n);
struct bork_entity* bork_entity_get(bork_entity_t ent);
void bork_entity_init(struct bork_entity* ent, enum bork_entity_type type);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map);

struct bork_map;
struct bork_play_data;

#define BORK_ENTFLAG_DEAD               (1 << 0)
#define BORK_ENTFLAG_INACTIVE           (1 << 1)
#define BORK_ENTFLAG_GROUND             (1 << 2)
#define BORK_ENTFLAG_SLIDE              (1 << 3)
#define BORK_ENTFLAG_CROUCH             (1 << 4)
#define BORK_ENTFLAG_LOOKED_AT          (1 << 5)
#define BORK_ENTFLAG_ITEM               (1 << 6)
#define BORK_ENTFLAG_PLAYER             (1 << 7)
#define BORK_ENTFLAG_ENEMY              (1 << 8)
#define BORK_ENTFLAG_DESTROY_ON_USE     (1 << 9)
#define BORK_ENTFLAG_STACKS             (1 << 10)
#define BORK_ENTFLAG_NOT_INTERACTIVE    (1 << 11)
#define BORK_ENTFLAG_IN_INVENTORY       (1 << 12)
#define BORK_ENTFLAG_DYING              (1 << 13)

struct bork_entity {
    vec3 pos;
    vec3 vel;
    vec2 dir;
    uint32_t flags;
    int dead_ticks;
    int still_ticks;
    int pain_ticks;
    int HP;
    int counter[4];
    int item_quantity;
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENTITY_ENEMY,
        BORK_ITEM_FIRSTAID,
        BORK_ITEM_BULLETS,
        BORK_ITEM_SHELLS,
        BORK_ITEM_PLAZMA,
        BORK_ITEM_SCRAPMETAL,
        BORK_ITEM_PISTOL,
        BORK_ITEM_SHOTGUN,
        BORK_ITEM_MACHINEGUN,
        BORK_ITEM_PLAZGUN,
        BORK_ITEM_LEADPIPE,
        BORK_ITEM_BEAMSWORD,
        BORK_ITEM_PLANT1,
        BORK_ITEM_DATAPAD,
        BORK_ENTITY_TYPES,
    } type;
};

void bork_use_pistol(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_pistol(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_shotgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_shotgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_machinegun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_machinegun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_plazgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_plazgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_firstaid(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_leadpipe(struct bork_entity* ent, struct bork_play_data* d);

static const struct bork_entity_profile {
    char name[32];
    uint32_t base_flags;
    int base_hp;
    vec3 size;
    vec4 sprite_tx;
    int front_frame;
    int dir_frames;
    uint8_t use_ctrl;
    float hud_angle;
    void (*use_func)(struct bork_entity*, struct bork_play_data*);
    void (*hud_func)(struct bork_entity*, struct bork_play_data*);
} BORK_ENT_PROFILES[] = {
    [BORK_ENTITY_PLAYER] = { .name = "Player",
        .base_flags = BORK_ENTFLAG_PLAYER,
        .size = { 0.5, 0.5, 0.9 },
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ENTITY_ENEMY] = { .name = "Enemy",
        .base_flags = BORK_ENTFLAG_ENEMY,
        .base_hp = 30,
        .size = { 1, 1, 1 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 192,
        .dir_frames = 4 },
    [BORK_ITEM_FIRSTAID] = { .name = "FIRST AID KIT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_DESTROY_ON_USE | BORK_ENTFLAG_STACKS,
        .size = { 0.5, 0.5, 0.5 },
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 0,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_firstaid },
    [BORK_ITEM_BULLETS] = { .name = "BULLETS",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 1 },
    [BORK_ITEM_SHELLS] = { .name = "SHELLS",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 1 },
    [BORK_ITEM_PLAZMA] = { .name = "PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 5 },
    [BORK_ITEM_PISTOL] = { .name = "PISTOL",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 16,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_pistol,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_SHOTGUN] = { .name = "SHOTGUN",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1.5, 1, 0, 0 },
        .front_frame = 14,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_shotgun,
        .hud_func = bork_hud_shotgun },
    [BORK_ITEM_MACHINEGUN] = { .name = "MACHINE GUN",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 8,
        .use_ctrl = PG_CONTROL_HELD,
        .use_func = bork_use_machinegun,
        .hud_func = bork_hud_machinegun },
    [BORK_ITEM_PLAZGUN] = { .name = "PLAZ-GUN",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 12,
        .use_ctrl = PG_CONTROL_HELD,
        .use_func = bork_use_plazgun,
        .hud_func = bork_hud_plazgun },
    [BORK_ITEM_LEADPIPE] = { .name = "STEEL ROD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 10,
        .hud_angle = -0.75,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_leadpipe },
    [BORK_ITEM_BEAMSWORD] = { .name = "BEAM SWORD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 18,
        .hud_angle = -0.75,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_leadpipe },
    [BORK_ITEM_PLANT1] = { .name = "PLANT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_NOT_INTERACTIVE,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 3 },
    [BORK_ITEM_SCRAPMETAL] = { .name = "SCRAP METAL",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 2 },
    [BORK_ITEM_DATAPAD] = { .name = "DATA PAD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 4 },
};

typedef int bork_entity_t;
bork_entity_t bork_entity_new(int n);
struct bork_entity* bork_entity_get(bork_entity_t ent);
void bork_entpool_clear(void);
void bork_entity_init(struct bork_entity* ent, enum bork_entity_type type);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map);
void bork_entity_get_eye(struct bork_entity* ent, vec3 out_dir, vec3 out_pos);

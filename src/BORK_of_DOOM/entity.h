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
#define BORK_ENTFLAG_SMOKING            (1 << 14)
#define BORK_ENTFLAG_ON_FIRE            (1 << 15)
#define BORK_ENTFLAG_IS_GUN             (1 << 16)
#define BORK_ENTFLAG_IS_MELEE           (1 << 17)
#define BORK_ENTFLAG_IS_AMMO            (1 << 18)
#define BORK_ENTFLAG_EMP                (1 << 19)
#define BORK_ENTFLAG_IS_RAW_MATERIAL    (1 << 20)
#define BORK_ENTFLAG_IS_FOOD            (1 << 21)
#define BORK_ENTFLAG_IS_ELECTRICAL      (1 << 22)
#define BORK_ENTFLAG_IS_CHEMICAL        (1 << 23)

struct bork_entity {
    int last_tick;
    vec3 pos;
    vec3 vel;
    vec2 dir;
    vec3 dst_pos;
    uint32_t flags;
    int path_ticks;
    int dead_ticks;
    int still_ticks;
    int pain_ticks;
    int aware_ticks;
    int emp_ticks;
    int fire_ticks;
    int HP;
    int ammo;
    int ammo_type;
    int counter[4];
    int item_quantity;
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENTITY_ENEMY,
        BORK_ITEM_PLANT1,
        BORK_ITEM_FIRSTAID,
        BORK_ITEM_DATAPAD,
        BORK_ITEM_BULLETS,
        BORK_ITEM_BULLETS_AP,
        BORK_ITEM_BULLETS_INC,
        BORK_ITEM_SHELLS,
        BORK_ITEM_SHELLS_AP,
        BORK_ITEM_SHELLS_INC,
        BORK_ITEM_PLAZMA,
        BORK_ITEM_PLAZMA_SUPER,
        BORK_ITEM_PLAZMA_ICE,
        BORK_ITEM_PISTOL,
        BORK_ITEM_SHOTGUN,
        BORK_ITEM_MACHINEGUN,
        BORK_ITEM_PLAZGUN,
        BORK_ITEM_KNIFE,
        BORK_ITEM_LEADPIPE,
        BORK_ITEM_BEAMSWORD,
        BORK_ITEM_SCRAPMETAL,
        BORK_ITEM_WIRES,
        BORK_ITEM_STEELPLATE,
        BORK_ITEM_UPGRADE,
        BORK_ITEM_DRINK_CUP,
        BORK_ITEM_DRINK_MUG,
        BORK_ITEM_FOOD_SOUP,
        BORK_ITEM_FOOD_SANDWICH,
        BORK_ITEM_FOOD_BREAD,
        BORK_ITEM_FOOD_WRAP,
        BORK_ITEM_PAN,
        BORK_ITEM_POT,
        BORK_ITEM_MICROSCOPE,
        BORK_ITEM_FLASK,
        BORK_ITEM_BEAKER,
        BORK_ITEM_VIALS,
        BORK_ITEM_BOOMBOX,
        BORK_ITEM_BATTERY,
        BORK_ITEM_FUEL,
        BORK_ITEM_SOLARCELL,
        BORK_ITEM_AIRFILTER,
        BORK_ITEM_BROKEN_HELMET,
        BORK_ITEM_OXYGEN_TANK,
        BORK_ITEM_CHEMICAL_TANK,
        BORK_ITEM_STOOL,
        BORK_ENTITY_TYPES,
    } type;
};
#define BORK_AMMO_TYPES 9

void bork_use_pistol(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_pistol(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_shotgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_shotgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_machinegun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_machinegun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_plazgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_hud_plazgun(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_firstaid(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_melee(struct bork_entity* ent, struct bork_play_data* d);

static const struct bork_entity_profile {
    char name[32];
    uint32_t base_flags;
    int base_hp;
    vec3 size;
    vec4 sprite_tx;
    float inv_height, inv_angle;
    int front_frame;
    int dir_frames;
    int reload_time;
    int speed;
    int damage;
    int armor_pierce;
    int ammo_capacity;
    enum bork_entity_type use_ammo[4];
    int ammo_types;
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
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 65,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_firstaid },
    [BORK_ITEM_BULLETS] = { .name = "BULLETS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 30,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 0 },
    [BORK_ITEM_BULLETS_AP] = { .name = "ARMOR-PIERCING BULLETS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 30,
        .armor_pierce = 10,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 16 },
    [BORK_ITEM_BULLETS_INC] = { .name = "INCENDIARY BULLETS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 30,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 32 },
    [BORK_ITEM_SHELLS] = { .name = "SHELLS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 12,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 1 },
    [BORK_ITEM_SHELLS_AP] = { .name = "ARMOR-PIERCING SHELLS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 12,
        .armor_pierce = 10,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 17 },
    [BORK_ITEM_SHELLS_INC] = { .name = "INCENDIARY SHELLS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 12,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 18 },
    [BORK_ITEM_PLAZMA] = { .name = "PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 20,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 2 },
    [BORK_ITEM_PLAZMA_SUPER] = { .name = "SUPER PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 20,
        .damage = 20,
        .armor_pierce = 10,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 18 },
    [BORK_ITEM_PLAZMA_ICE] = { .name = "FREEZE PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 20,
        .damage = -10,
        .armor_pierce = -10,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 34 },
    [BORK_ITEM_PISTOL] = { .name = "PISTOL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 52,
        .reload_time = 120,
        .speed = 5,
        .damage = 10,
        .armor_pierce = 5,
        .ammo_capacity = 12,
        .use_ammo = { BORK_ITEM_BULLETS, BORK_ITEM_BULLETS_AP, BORK_ITEM_BULLETS_INC },
        .ammo_types = 3,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_pistol,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_SHOTGUN] = { .name = "SHOTGUN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1.5, 1, 0, 0 },
        .front_frame = 19,
        .reload_time = 300,
        .speed = 2,
        .damage = 5,
        .armor_pierce = 0,
        .ammo_capacity = 6,
        .use_ammo = { BORK_ITEM_SHELLS, BORK_ITEM_SHELLS_AP, BORK_ITEM_SHELLS_INC },
        .ammo_types = 3,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_shotgun,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_MACHINEGUN] = { .name = "MACHINE GUN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 3,
        .reload_time = 180,
        .speed = 10,
        .damage = 10,
        .armor_pierce = 5,
        .ammo_capacity = 30,
        .use_ammo = { BORK_ITEM_BULLETS, BORK_ITEM_BULLETS_AP, BORK_ITEM_BULLETS_INC },
        .ammo_types = 3,
        .use_ctrl = PG_CONTROL_HELD,
        .use_func = bork_use_machinegun,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_PLAZGUN] = { .name = "PLAZ-GUN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 35,
        .reload_time = 360,
        .damage = 20,
        .armor_pierce = 10,
        .ammo_capacity = 20,
        .use_ammo = { BORK_ITEM_PLAZMA, BORK_ITEM_PLAZMA_SUPER, BORK_ITEM_PLAZMA_ICE },
        .ammo_types = 3,
        .use_ctrl = PG_CONTROL_HELD,
        .use_func = bork_use_plazgun,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_KNIFE] = { .name = "COMBAT KNIFE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_MELEE,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 37,
        .inv_height = 0.5,
        .inv_angle = 0.3,
        .hud_angle = -0.75,
        .speed = 5,
        .damage = 10,
        .armor_pierce = 0,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_melee },
    [BORK_ITEM_LEADPIPE] = { .name = "STEEL ROD",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_MELEE,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 5,
        .inv_height = 0.5,
        .inv_angle = 0.3,
        .hud_angle = -0.75,
        .speed = 3,
        .damage = 10,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_melee },
    [BORK_ITEM_BEAMSWORD] = { .name = "BEAM SWORD",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_MELEE,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 21,
        .inv_height = 0.5,
        .inv_angle = 0.3,
        .hud_angle = -0.75,
        .speed = 4,
        .damage = 15,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_melee },
    [BORK_ITEM_PLANT1] = { .name = "PLANT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_NOT_INTERACTIVE,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 80 },
    [BORK_ITEM_SCRAPMETAL] = { .name = "CHUNK OF METAL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 48 },
    [BORK_ITEM_WIRES] = { .name = "FRAYED WIRING",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 49 },
    [BORK_ITEM_STEELPLATE] = { .name = "STEEL PLATING",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 50 },
    [BORK_ITEM_DATAPAD] = { .name = "DATA PAD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 66 },
    [BORK_ITEM_UPGRADE] = { .name = "TECH UPGRADE",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.4 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 64 },
    [BORK_ITEM_DRINK_CUP] = { .name = "DRINK CUP",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 112,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_DRINK_MUG] = { .name = "DRINK MUG",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 113,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_SOUP] = { .name = "SOUP BOWL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 114,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_SANDWICH] = { .name = "SANDWICH",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 115,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_BREAD] = { .name = "BREAD ROLL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 116,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_WRAP] = { .name = "LUNCH WRAP",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 117,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_PAN] = { .name = "COOKING PAN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 128,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_POT] = { .name = "COOKING POT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 129,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_MICROSCOPE] = { .name = "MICROSCOPE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 130,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FLASK] = { .name = "LAB FLASK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 131,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BEAKER] = { .name = "LAB BEAKER",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 132,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_VIALS] = { .name = "VIAL TRAY",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 133,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_AIRFILTER] = { .name = "AIR FILTER",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 176,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BROKEN_HELMET] = { .name = "BROKEN HELMET",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 177,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BOOMBOX] = { .name = "MUSIC PLAYER",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_ELECTRICAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 160,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BATTERY] = { .name = "BATTERY CELL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_ELECTRICAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 161,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_SOLARCELL] = { .name = "SOLAR CELL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_ELECTRICAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 163,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FUEL] = { .name = "FUEL TANK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_CHEMICAL,
        .size = { 0.4, 0.4, 0.4 },
        .front_frame = 162,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_OXYGEN_TANK] = { .name = "OXYGEN TANK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_CHEMICAL,
        .size = { 0.4, 0.4, 0.6 },
        .front_frame = 192,
        .sprite_tx = { 1, 1.5, 0, 0 } },
    [BORK_ITEM_CHEMICAL_TANK] = { .name = "CHEMICAL TANK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_CHEMICAL,
        .size = { 0.4, 0.4, 0.6 },
        .front_frame = 193,
        .sprite_tx = { 1, 1.5, 0, 0 } },
    [BORK_ITEM_STOOL] = { .name = "STOOL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_NOT_INTERACTIVE,
        .size = { 0.5, 0.5, 0.5 },
        .front_frame = 194,
        .sprite_tx = { 1, 1, 0, 0 } },
};

typedef int bork_entity_t;
typedef ARR_T(bork_entity_t) bork_entity_arr_t;
bork_entity_t bork_entity_new(int n);
struct bork_entity* bork_entity_get(bork_entity_t ent);
void bork_entpool_clear(void);
void bork_entity_init(struct bork_entity* ent, enum bork_entity_type type);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map);
void bork_entity_get_view(struct bork_entity* ent, mat4 view);
void bork_entity_get_eye(struct bork_entity* ent, vec3 out_dir, vec3 out_pos);

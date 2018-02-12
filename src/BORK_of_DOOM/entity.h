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
#define BORK_ENTFLAG_BOUNCE             (1 << 24)
#define BORK_ENTFLAG_ACTIVE_FUNC        (1 << 25)
#define BORK_ENTFLAG_FLIES              (1 << 26)
#define BORK_ENTFLAG_STATIONARY         (1 << 27)
#define BORK_ENTFLAG_ENTITY             (1 << 28)
#define BORK_ENTFLAG_FIREPROOF          (1 << 29)
#define BORK_ENTFLAG_SOUND              (1 << 30)

struct bork_entity {
    int last_tick;
    vec3 pos;
    vec3 vel;
    vec2 dir;
    vec3 dst_pos;
    uint32_t flags;
    int freeze_ticks;
    int path_ticks;
    int dead_ticks;
    int still_ticks;
    int pain_ticks;
    int aware_ticks;
    int emp_ticks;
    int fire_ticks;
    int last_fire_tick;
    int HP;
    int ammo;
    int ammo_type;
    int counter[4];
    int item_quantity;
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENEMY_TEST,
        BORK_ENEMY_TIN_CANINE,
        BORK_ENEMY_BOTTWEILER,
        BORK_ENEMY_FANG_BANGER,
        BORK_ENEMY_SNOUT_DRONE,
        BORK_ENEMY_GREAT_BANE,
        BORK_ENEMY_LAIKA,
        BORK_ITEM_PLANT1,
        BORK_ITEM_DATAPAD,
        BORK_ITEM_UPGRADE,
        BORK_ITEM_SCHEMATIC,
        BORK_ITEM_FIRSTAID,
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
        BORK_ITEM_STEELPLATE,
        BORK_ITEM_WIRES,
        BORK_ITEM_BEARTRAP,
        BORK_ITEM_GRENADE_EMP,
        BORK_ITEM_GRENADE_FRAG,
        BORK_ITEM_GRENADE_INC,
        BORK_ITEM_PROXY_FRAG,
        BORK_ITEM_PROXY_EMP,
        BORK_ITEM_DOOR_OVERRIDE,
        BORK_ITEM_FIRE_EXTINGUISHER,
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
        BORK_ITEM_BATTERY,
        BORK_ITEM_FUEL,
        BORK_ITEM_SOLARCELL,
        BORK_ITEM_AIRFILTER,
        BORK_ITEM_BROKEN_HELMET,
        BORK_ITEM_OXYGEN_TANK,
        BORK_ITEM_CHEMICAL_TANK,
        BORK_ITEM_CHEW_TOY,
        BORK_ITEM_DOGFOOD,
        BORK_ITEM_BONE,
        BORK_ITEM_SHOCK_PROD,
        BORK_ITEM_SHOCK_PROD_USED,
        BORK_ENTITY_BOOMBOX,
        BORK_ENTITY_DESKLAMP,
        BORK_ENTITY_STOOL,
        BORK_ENTITY_BARREL,
        BORK_SOUND_HUM,
        BORK_SOUND_HISS,
        BORK_SOUND_COMPUTERS,
        BORK_SOUND_BUZZ,
        BORK_SOUND_HUM2,
        BORK_SOUND_HUM3,
        BORK_ENTITY_TYPES,
    } type;
};
#define BORK_AMMO_TYPES 9

void bork_use_door_override(struct bork_entity* ent, struct bork_play_data* d);
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
void bork_use_prod(struct bork_entity* ent, struct bork_play_data* d);
void bork_use_grenade(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_grenade_emp(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_grenade_inc(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_grenade(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_test_enemy(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_tin_canine(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_bottweiler(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_fang_banger(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_snout_drone(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_great_bane(struct bork_entity* ent, struct bork_play_data* d);
void bork_tick_laika(struct bork_entity* ent, struct bork_play_data* d);
void bork_barrel_explode(struct bork_entity* ent, struct bork_play_data* d);
void bork_electronic_die(struct bork_entity* ent, struct bork_play_data* d);
void bork_stool_die(struct bork_entity* ent, struct bork_play_data* d);

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
    void (*tick_func)(struct bork_entity*, struct bork_play_data*);
    void (*death_func)(struct bork_entity*, struct bork_play_data*);
    void (*use_func)(struct bork_entity*, struct bork_play_data*);
    void (*hud_func)(struct bork_entity*, struct bork_play_data*);
} BORK_ENT_PROFILES[] = {
    [BORK_SOUND_HUM] = { .name = "SOUND_HUM",
        .base_flags = BORK_ENTFLAG_SOUND, },
    [BORK_SOUND_HUM2] = { .name = "SOUND_HUM2",
        .base_flags = BORK_ENTFLAG_SOUND, },
    [BORK_SOUND_HUM3] = { .name = "SOUND_HUM3",
        .base_flags = BORK_ENTFLAG_SOUND, },
    [BORK_SOUND_HISS] = { .name = "SOUND_HISS",
        .base_flags = BORK_ENTFLAG_SOUND, },
    [BORK_SOUND_COMPUTERS] = { .name = "SOUND_COMPUTERS",
        .base_flags = BORK_ENTFLAG_SOUND, },
    [BORK_SOUND_BUZZ] = { .name = "SOUND_BUZZ",
        .base_flags = BORK_ENTFLAG_SOUND, },
    [BORK_ENTITY_PLAYER] = { .name = "Player",
        .base_flags = BORK_ENTFLAG_PLAYER,
        .size = { 0.5, 0.5, 0.9 },
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ENEMY_TEST] = { .name = "TEST ENEMY",
        .base_flags = BORK_ENTFLAG_ENEMY,
        .base_hp = 100,
        .size = { 1, 1, 1 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 0,
        .dir_frames = 8,
        .tick_func = bork_tick_test_enemy },
    [BORK_ENEMY_TIN_CANINE] = { .name = "TIN CANINE",
        .base_flags = BORK_ENTFLAG_ENEMY,
        .base_hp = 150,
        .size = { 0.9, 0.9, 0.9 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 0,
        .dir_frames = 8,
        .tick_func = bork_tick_tin_canine },
    [BORK_ENEMY_BOTTWEILER] = { .name = "BOTTWEILER",
        .base_flags = BORK_ENTFLAG_ENEMY,
        .base_hp = 50,
        .size = { 0.75, 0.75, 0.75 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 16,
        .dir_frames = 8,
        .tick_func = bork_tick_bottweiler },
    [BORK_ENEMY_FANG_BANGER] = { .name = "FANG BANGER",
        .base_flags = BORK_ENTFLAG_ENEMY | BORK_ENTFLAG_FIREPROOF,
        .base_hp = 25,
        .size = { 0.5, 0.5, 0.5 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 32,
        .dir_frames = 8,
        .tick_func = bork_tick_fang_banger },
    [BORK_ENEMY_SNOUT_DRONE] = { .name = "SNOUT DRONE",
        .base_flags = BORK_ENTFLAG_ENEMY | BORK_ENTFLAG_FLIES,
        .base_hp = 100,
        .size = { 1, 1, 1 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 48,
        .dir_frames = 8,
        .tick_func = bork_tick_snout_drone },
    [BORK_ENEMY_GREAT_BANE] = { .name = "GREAT BANE",
        .base_flags = BORK_ENTFLAG_ENEMY | BORK_ENTFLAG_STATIONARY,
        .base_hp = 500,
        .size = { 1.5, 1.5, 1.5 },
        .sprite_tx = { 2, 2, 0, 0 },
        .front_frame = 64,
        .dir_frames = 8,
        .tick_func = bork_tick_great_bane },
    [BORK_ENEMY_LAIKA] = { .name = "LAIKA",
        .base_flags = BORK_ENTFLAG_ENEMY | BORK_ENTFLAG_STATIONARY,
        .base_hp = 1500,
        .size = { 1, 1, 1.25 },
        .sprite_tx = { 2, 1.5, 0, 0 },
        .front_frame = 96,
        .dir_frames = 8,
        .tick_func = bork_tick_laika },
    [BORK_ENTITY_STOOL] = { .name = "STOOL",
        .base_flags = BORK_ENTFLAG_ENTITY,
        .base_hp = 20,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 178,
        .sprite_tx = { 1, 1, 0, 0 },
        .death_func = bork_stool_die },
    [BORK_ENTITY_BOOMBOX] = { .name = "MUSIC PLAYER",
        .base_flags = BORK_ENTFLAG_ENTITY,
        .base_hp = 5,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 160,
        .sprite_tx = { 1, 1, 0, 0 },
        .death_func = bork_electronic_die },
    [BORK_ENTITY_DESKLAMP] = { .name = "DESK LAMP",
        .base_flags = BORK_ENTFLAG_ENTITY,
        .base_hp = 5,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 164,
        .sprite_tx = { 1, 1, 0, 0 },
        .death_func = bork_electronic_die },
    [BORK_ENTITY_BARREL] = { .name = "WASTE BARREL",
        .base_flags = BORK_ENTFLAG_ENTITY,
        .base_hp = 10,
        .size = { 0.3, 0.3, 0.5 },
        .front_frame = 195,
        .sprite_tx = { 1, 1.5, 0, 0 },
        .death_func = bork_barrel_explode },
    [BORK_ITEM_DATAPAD] = { .name = "DATA PAD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 66 },
    [BORK_ITEM_UPGRADE] = { .name = "TECH UPGRADE",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 64 },
    [BORK_ITEM_SCHEMATIC] = { .name = "SCHEMATIC",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 82 },
    [BORK_ITEM_FIRSTAID] = { .name = "FIRST AID KIT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_DESTROY_ON_USE | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 65,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_firstaid },
    [BORK_ITEM_BULLETS] = { .name = "BULLETS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 30,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 0 },
    [BORK_ITEM_BULLETS_AP] = { .name = "BULLETS (AP)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 30,
        .damage = 12,
        .armor_pierce = 10,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 16 },
    [BORK_ITEM_BULLETS_INC] = { .name = "BULLETS (INC.)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 30,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 32 },
    [BORK_ITEM_SHELLS] = { .name = "SHELLS",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 12,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 1 },
    [BORK_ITEM_SHELLS_AP] = { .name = "SHELLS (AP)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 12,
        .damage = 10,
        .armor_pierce = 10,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 17 },
    [BORK_ITEM_SHELLS_INC] = { .name = "SHELLS (INC.)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 12,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 33 },
    [BORK_ITEM_PLAZMA] = { .name = "PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 20,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 2 },
    [BORK_ITEM_PLAZMA_SUPER] = { .name = "SUPER PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 20,
        .damage = 20,
        .armor_pierce = 10,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 18 },
    [BORK_ITEM_PLAZMA_ICE] = { .name = "FREEZE PLAZMA",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_AMMO,
        .ammo_capacity = 20,
        .damage = -15,
        .armor_pierce = -10,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 34 },
    [BORK_ITEM_PISTOL] = { .name = "PISTOL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.3, 0.3, 0.3 },
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
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 19,
        .reload_time = 300,
        .speed = 2,
        .damage = 8,
        .armor_pierce = 0,
        .ammo_capacity = 6,
        .use_ammo = { BORK_ITEM_SHELLS, BORK_ITEM_SHELLS_AP, BORK_ITEM_SHELLS_INC },
        .ammo_types = 3,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_shotgun,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_MACHINEGUN] = { .name = "MACHINE GUN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 3,
        .reload_time = 180,
        .speed = 10,
        .damage = 15,
        .armor_pierce = 5,
        .ammo_capacity = 30,
        .use_ammo = { BORK_ITEM_BULLETS, BORK_ITEM_BULLETS_AP, BORK_ITEM_BULLETS_INC },
        .ammo_types = 3,
        .use_ctrl = PG_CONTROL_HELD,
        .use_func = bork_use_machinegun,
        .hud_func = bork_hud_pistol },
    [BORK_ITEM_PLAZGUN] = { .name = "PLAZ-GUN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_GUN,
        .size = { 0.3, 0.3, 0.3 },
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
        .size = { 0.3, 0.3, 0.3 },
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
        .size = { 0.3, 0.3, 0.3 },
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
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 2, 1, 0, 0 },
        .front_frame = 21,
        .inv_height = 0.5,
        .inv_angle = 0.3,
        .hud_angle = -0.75,
        .speed = 4,
        .damage = 25,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_melee },
    [BORK_ITEM_PLANT1] = { .name = "PLANT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_NOT_INTERACTIVE,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 80 },
    [BORK_ITEM_SCRAPMETAL] = { .name = "CHUNK OF METAL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 48 },
    [BORK_ITEM_WIRES] = { .name = "FRAYED WIRING",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_ELECTRICAL,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 49 },
    [BORK_ITEM_STEELPLATE] = { .name = "STEEL PLATING",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 50 },
    [BORK_ITEM_BEARTRAP] = { .name = "BEAR TRAP",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 40 },
    [BORK_ITEM_GRENADE_FRAG] = { .name = "GRENADE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 7,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_grenade,
        .tick_func = bork_tick_grenade },
    [BORK_ITEM_GRENADE_EMP] = { .name = "GRENADE (EMP)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 23,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_grenade,
        .tick_func = bork_tick_grenade_emp },
    [BORK_ITEM_GRENADE_INC] = { .name = "GRENADE (INC.)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 39,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_grenade,
        .tick_func = bork_tick_grenade_inc },
    [BORK_ITEM_PROXY_FRAG] = { .name = "PROXY MINE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 8 },
    [BORK_ITEM_PROXY_EMP] = { .name = "PROXY MINE (EMP)",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .front_frame = 24 },
    [BORK_ITEM_DOOR_OVERRIDE] = { .name = "DOOR OVERRIDE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .sprite_tx = { 1, 1, 0, 0 },
        .inv_angle = 0.3,
        .inv_height = 0.4,
        .front_frame = 68,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_door_override },
    [BORK_ITEM_FIRE_EXTINGUISHER] = { .name = "FIRE EXTINGUISHER",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.4, 0.4, 0.6 },
        .sprite_tx = { 1, 1.5, 0, 0 },
        .inv_angle = -1,
        .front_frame = 194 },
    [BORK_ITEM_DRINK_CUP] = { .name = "DRINK CUP",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 112,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_DRINK_MUG] = { .name = "DRINK MUG",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 113,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_SOUP] = { .name = "SOUP BOWL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 114,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_SANDWICH] = { .name = "SANDWICH",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 115,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_BREAD] = { .name = "BREAD ROLL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 116,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FOOD_WRAP] = { .name = "LUNCH WRAP",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 117,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_PAN] = { .name = "COOKING PAN",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 128,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_POT] = { .name = "COOKING POT",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 129,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_MICROSCOPE] = { .name = "MICROSCOPE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 130,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FLASK] = { .name = "LAB FLASK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 131,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BEAKER] = { .name = "LAB BEAKER",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 132,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_VIALS] = { .name = "VIAL TRAY",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 133,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_AIRFILTER] = { .name = "AIR FILTER",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_RAW_MATERIAL | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 176,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BROKEN_HELMET] = { .name = "BROKEN HELMET",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_RAW_MATERIAL | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 177,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BATTERY] = { .name = "BATTERY CELL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_ELECTRICAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 161,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_SOLARCELL] = { .name = "SOLAR CELL",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_ELECTRICAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 163,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_FUEL] = { .name = "FUEL TANK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_CHEMICAL | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 162,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_OXYGEN_TANK] = { .name = "OXYGEN TANK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_CHEMICAL | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.5 },
        .front_frame = 192,
        .inv_angle = -1,
        .sprite_tx = { 1, 1.5, 0, 0 } },
    [BORK_ITEM_CHEMICAL_TANK] = { .name = "CHEMICAL TANK",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_IS_CHEMICAL | BORK_ENTFLAG_STACKS,
        .size = { 0.3, 0.3, 0.5 },
        .front_frame = 193,
        .inv_angle = -1,
        .sprite_tx = { 1, 1.5, 0, 0 } },
    [BORK_ITEM_CHEW_TOY] = { .name = "CHEW TOY",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_RAW_MATERIAL,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 134,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_DOGFOOD] = { .name = "DOG FOOD",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 118,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_BONE] = { .name = "BONE",
        .base_flags = BORK_ENTFLAG_ITEM | BORK_ENTFLAG_STACKS | BORK_ENTFLAG_IS_FOOD,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 119,
        .sprite_tx = { 1, 1, 0, 0 } },
    [BORK_ITEM_SHOCK_PROD] = { .name = "SHOCK PROD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 53,
        .sprite_tx = { 2, 1, 0, 0 },
        .inv_height = 0.5,
        .inv_angle = 0.3,
        .hud_angle = -0.75,
        .damage = 20,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_prod },
    [BORK_ITEM_SHOCK_PROD_USED] = { .name = "DISCHARGED SHOCK PROD",
        .base_flags = BORK_ENTFLAG_ITEM,
        .size = { 0.3, 0.3, 0.3 },
        .front_frame = 69,
        .sprite_tx = { 2, 1, 0, 0 },
        .inv_height = 0.5,
        .inv_angle = 0.3,
        .hud_angle = -0.75,
        .damage = 5,
        .speed = 3,
        .use_ctrl = PG_CONTROL_HIT,
        .use_func = bork_use_melee },
};

typedef int bork_entity_t;
typedef ARR_T(bork_entity_t) bork_entity_arr_t;
size_t bork_entpool_write_to_file(FILE* f);
size_t bork_entpool_read_from_file(FILE* f);
bork_entity_t bork_entity_new(int n);
struct bork_entity* bork_entity_get(bork_entity_t ent);
bork_entity_t bork_entity_id(struct bork_entity* ent);
void bork_entpool_clear(void);
void bork_entity_init(struct bork_entity* ent, enum bork_entity_type type);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_move(struct bork_entity* ent, struct bork_play_data* d);
void bork_entity_update(struct bork_entity* ent, struct bork_play_data* d);
void bork_entity_get_view(struct bork_entity* ent, mat4 view);
void bork_entity_get_eye(struct bork_entity* ent, vec3 out_dir, vec3 out_pos);
void bork_entity_look_at(struct bork_entity* ent, vec3 look);
void bork_entity_look_dir(struct bork_entity* ent, vec3 look);
void bork_entity_turn_toward(struct bork_entity* ent, vec3 look, float rate);

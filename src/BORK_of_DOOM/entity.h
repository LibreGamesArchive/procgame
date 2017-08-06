struct bork_map;

#define BORK_ENTFLAG_DEAD       (1 << 0)
#define BORK_ENTFLAG_ACTIVE     (1 << 1)
#define BORK_ENTFLAG_GROUND     (1 << 2)
#define BORK_ENTFLAG_LOOKED_AT  (1 << 3)

struct bork_entity {
    char name[32];
    /*  The basics   */
    vec3 pos;
    vec3 size;
    vec3 vel;
    vec2 dir;
    /*  Rendering stuff */
    vec4 sprite_tx;
    int front_frame;
    int dir_frames;
    /*  Gameplay stuff  */
    uint32_t flags;
    int still_ticks;
    int HP;
    uint8_t counter[4];
    /*  Different types of items have special data  */
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENTITY_ENEMY,
        BORK_ENTITY_ITEM
    } type;
    enum bork_item_type {
        BORK_ITEM_DOGFOOD,
        BORK_ITEM_BULLETS,
        BORK_ITEM_SCRAP_METAL,
        BORK_ITEM_MACHINEGUN,
    } item_type;
};

typedef int bork_entity_t;
bork_entity_t bork_entity_new(int n);
struct bork_entity* bork_entity_get(bork_entity_t ent);
void bork_entity_init(struct bork_entity* ent, vec3 size);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map);

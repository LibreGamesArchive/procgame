struct bork_map;

struct bork_entity {
    char name[32];
    vec3 pos;
    vec3 size;
    vec3 vel;
    vec2 dir;
    int ground;
    int front_frame;
    int dir_frames;
    int dead;
    int active;
    int still_ticks;
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENTITY_ENEMY,
        BORK_ENTITY_ITEM
    } type;
    union {
        struct {
            int health;
        } enemy;
        struct {
            int looked_at;
        } item;
    };
};

void bork_entity_init(struct bork_entity* ent, vec3 size);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map);

struct bork_map;

struct bork_entity {
    vec3 pos;
    vec3 size;
    vec3 vel;
    vec2 dir;
    int ground;
    int front_frame;
    int dir_frames;
    int dead;
    enum bork_entity_type {
        BORK_ENTITY_PLAYER,
        BORK_ENTITY_ENEMY,
        BORK_ENTITY_ITEM
    } type;
    union {
        struct {
            int health;
        } enemy;
    };
};

void bork_entity_init(struct bork_entity* ent, vec3 size);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_update(struct bork_entity* ent, struct bork_map* map);
void bork_entity_begin(enum bork_entity_type type, struct bork_game_core* core);
void bork_entity_draw_enemy(struct bork_entity* ent, struct bork_game_core* core);

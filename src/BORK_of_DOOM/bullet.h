#define BORK_BULLET_DEAD            (1 << 0)
#define BORK_BULLET_HURTS_PLAYER    (1 << 1)
#define BORK_BULLET_HURTS_ENEMY     (1 << 2)

struct bork_bullet {
    uint16_t flags;
    uint8_t dead_ticks;
    uint8_t type;
    uint8_t damage;
    vec3 light_color;
    vec3 pos;
    vec3 dir;
    float range, dist_moved;
};

void bork_bullet_init(struct bork_bullet* ent, vec3 pos, vec3 dir);
void bork_bullet_move(struct bork_bullet* blt, struct bork_play_data* d);

struct bork_bullet {
    int dead_ticks;
    uint32_t flags;
    vec3 light_color;
    vec3 pos;
    vec3 dir;
};

void bork_bullet_init(struct bork_bullet* ent, vec3 pos, vec3 dir);
void bork_bullet_move(struct bork_bullet* blt, struct bork_map* map);

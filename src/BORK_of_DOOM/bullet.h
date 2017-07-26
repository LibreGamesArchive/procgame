struct bork_bullet {
    unsigned dead_ticks;
    vec3 pos;
    vec3 dir;
};

void bork_bullet_init(struct bork_bullet* ent, vec3 pos, vec3 dir);
void bork_bullet_move(struct bork_bullet* blt, struct bork_map* map);

struct bork_entity {
    vec3 pos;
    vec3 size;
    vec3 vel;
    vec2 dir;
    int ground;
};

void bork_entity_init(struct bork_entity* ent, vec3 size);
void bork_entity_push(struct bork_entity* ent, vec3 push);
void bork_entity_move(struct bork_entity* ent, struct bork_map* map);

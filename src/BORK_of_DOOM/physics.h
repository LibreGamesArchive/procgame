struct bork_collision {
    int x, y, z;
    vec3 norm;
    struct bork_tile* tile;
};

int bork_map_collide(struct bork_map* map, struct bork_collision* coll_out,
                     vec3 out, vec3 pos, vec3 size);

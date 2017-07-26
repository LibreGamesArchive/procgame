struct bork_collision {
    int x, y, z;
    vec3 face_norm, push;
    struct bork_tile* tile;
    struct bork_map_object* obj;
};

int bork_map_collide(struct bork_map* map, struct bork_collision* coll_out,
                     vec3 pos, vec3 size);

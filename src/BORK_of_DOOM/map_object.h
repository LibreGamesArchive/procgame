struct bork_map_object {
    int x, y, z;
    enum {
        BORK_MAP_OBJ_DOOR,
        BORK_MAP_OBJ_JUMPPAD
    } type;
    union {
        struct {
            int locked;
            float pos;
        } door;
        struct {
            int locked;
            float power;
        } jumppad;
    };
};

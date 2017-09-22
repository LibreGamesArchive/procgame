struct bork_editor_tile {
    uint8_t type;
    uint8_t dir;
    uint16_t obj_idx;
};

struct bork_editor_data {
    struct bork_game_core* core;
    const uint8_t* kb_state;
    struct bork_editor_map {
        struct bork_editor_tile tiles[32][32][32];
        ARR_T(struct bork_editor_obj {
            uint8_t x, y, z;
            union {
                struct { uint8_t flags; uint8_t code[4]; } door;
            };
        }) objs;
        ARR_T(struct bork_editor_entity {
            uint8_t type;
            vec3 pos;
            int option[4];
        }) ents;
    } map;
    struct bork_editor_tile current_tile;
    int selected_ent;
    enum bork_entity_type ent_type;
    int upgrade_type[2];
    int datapad_id;
    int select_mode;    /*  Whether the selection is being drawn    */
    int selection[4];
    int cursor[3];
};

void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);
int bork_editor_load_map(struct bork_editor_map* map, char* filename);
void bork_editor_complete_map(struct bork_map* map, struct bork_editor_map* ed_map);

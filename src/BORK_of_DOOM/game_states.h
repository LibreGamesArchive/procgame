struct bork_editor_tile {
    uint8_t type;
    uint8_t alt_type;
    uint8_t dir;
    uint8_t alt_dir;
    uint8_t wall[6];
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
                struct { int id; } teleport;
                struct { uint8_t pos; uint8_t scale; char text[32];
                         char text_len; uint8_t color[4]; } text;
            };
        }) objs;
        ARR_T(struct bork_editor_entity {
            uint8_t type;
            vec3 pos;
            int option[4];
        }) ents;
    } map;
    struct bork_editor_tile current_tile;
    int text_input;
    int selected_ent;
    enum bork_entity_type ent_type;
    int upgrade_type[2];
    int datapad_id;
    int snd_volume;
    enum bork_schematic schematic_type;
    int select_mode;    /*  Whether the selection is being drawn    */
    int selection[4];
    int cursor[3];
};

void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);
int bork_editor_load_map(struct bork_editor_map* map, char* filename);
void bork_editor_complete_map(struct bork_map* map, struct bork_editor_map* ed_map, int newgame);

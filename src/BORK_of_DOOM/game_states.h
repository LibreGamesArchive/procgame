struct bork_light {
    vec4 pos;
    vec4 dir_angle;
    vec3 color;
};

struct bork_play_data {
    /*  Core data   */
    int ticks;
    struct bork_game_core* core;
    ARR_T(struct bork_light) lights_buf;    /*  Updated per frame   */
    ARR_T(struct bork_light) spotlights;
    /*  Gameplay data   */
    struct bork_map map;
    struct bork_entity plr;
    float player_speed;
    int held_item;
    int quick_item[4];
    int ammo_bullets;
    ARR_T(bork_entity_t) inventory;
    ARR_T(struct bork_bullet) bullets;
    /*  The HUD-item animation  */
    vec3 hud_anim[5];
    float hud_anim_progress;
    float hud_anim_speed;
    int hud_anim_active;
    int hud_anim_destroy_when_finished;
    /*  Input states    */
    struct {
        enum {
            BORK_MENU_CLOSED,
            BORK_MENU_INVENTORY,
            BORK_MENU_CHARACTER,
        } state;
        int selection_idx, scroll_idx;
    } menu;
    bork_entity_t looked_item;
    int looked_idx;
};
void bork_play_reset_hud_anim(struct bork_play_data* d);

struct bork_editor_tile {
    uint32_t type;
    uint8_t dir;
    int8_t light_pos[4];
    int8_t light_dir[4];
    int8_t light_color[4];
};

struct bork_editor_data {
    struct bork_game_core* core;
    const uint8_t* kb_state;
    struct bork_editor_map {
        struct bork_editor_tile tiles[32][32][32];
        ARR_T(struct bork_editor_entity {
            uint8_t type;
            vec3 pos;
        }) ents;
    } map;
    struct bork_editor_tile current_tile;
    int selected_ent;
    enum bork_entity_type ent_type;
    int select_mode;    /*  Whether the selection is being drawn    */
    int selection[4];
    int cursor[3];
};

void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);
int bork_editor_load_map(struct bork_editor_map* map, char* filename);
void bork_editor_complete_map(struct bork_map* map, struct bork_editor_map* ed_map);

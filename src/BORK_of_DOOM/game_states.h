struct bork_play_data {
    /*  Core data   */
    int ticks;
    struct bork_game_core* core;
    ARR_T(struct pg_light) lights_buf;    /*  Updated per frame   */
    ARR_T(struct pg_light) spotlights;
    /*  Gameplay data   */
    struct bork_map map;
    struct bork_entity plr;
    int flashlight_on;
    float player_speed;
    int held_item;
    int quick_item[4];
    int reload_ticks, reload_length;
    int ammo[BORK_AMMO_TYPES];
    ARR_T(bork_entity_t) inventory;
    ARR_T(struct bork_bullet) bullets;
    ARR_T(struct bork_particle) particles;
    /*  The HUD-item animation  */
    vec3 hud_anim[5];
    float hud_angle[5];
    float hud_anim_progress;
    float hud_anim_speed;
    int hud_anim_active;
    int hud_anim_destroy_when_finished;
    void (*hud_anim_callback)(struct bork_entity* item, struct bork_play_data* d);
    int hud_callback_frame;
    /*  HUD datapad info    */
    int hud_datapad_id;
    int hud_datapad_ticks;
    int hud_datapad_line;
    /*  Input states    */
    vec2 mouse_motion;
    int crouch_toggle;
    int joystick_held[2];
    int trigger_held[2];
    struct {
        enum {
            BORK_MENU_CLOSED,
            BORK_MENU_DOORPAD,
            BORK_MENU_INVENTORY,
            BORK_MENU_CHARACTER,
            BORK_MENU_PLAYERDEAD,
        } state;
        union {
            struct {
                int selection_idx, scroll_idx;
                int ammo_select;
            } inv;
            struct {
                int selection[2];
                int unlocked_ticks;
                int door_idx;
                int num_chars;
                uint8_t chars[8];
            } doorpad;
        };
    } menu;
    bork_entity_t looked_item;
    bork_entity_t looked_enemy;
};
void bork_play_reset_hud_anim(struct bork_play_data* d);

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
            int option;
        }) ents;
    } map;
    struct bork_editor_tile current_tile;
    int selected_ent;
    enum bork_entity_type ent_type;
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

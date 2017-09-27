#define PLAY_SECONDS(s)     ((int)(s * 120))
struct bork_play_data {
    /*  Core data   */
    int ticks;
    int play_ticks;
    struct bork_game_core* core;
    ARR_T(struct pg_light) lights_buf;    /*  Updated per frame   */
    ARR_T(struct pg_light) spotlights;
    /*  Gameplay data   */
    struct bork_map map;
    struct bork_entity plr;
    int plr_map_pos[3];
    int flashlight_on;
    float player_speed;
    int held_item;
    int quick_item[4];
    int reload_ticks, reload_length;
    int ammo[BORK_AMMO_TYPES];
    int jump_released;
    enum bork_upgrade upgrades[3];
    int upgrade_level[3];
    int upgrade_counters[3];
    int upgrade_use_level;
    int upgrade_selected;
    int decoy_active;
    vec3 decoy_pos;
    uint32_t held_schematics;
    bork_entity_t teleporter_item;
    bork_entity_arr_t plr_enemy_query;
    bork_entity_arr_t plr_item_query;
    bork_entity_arr_t inventory;
    bork_entity_arr_t held_upgrades;
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
            BORK_MENU_RECYCLER,
            BORK_MENU_INVENTORY,
            BORK_MENU_UPGRADES,
            BORK_MENU_PLAYERDEAD,
        } state;
        struct {
            int selection_idx, scroll_idx;
            int ammo_select;
        } inv;
        struct {
            struct bork_map_obj* obj;
            int selection_idx, scroll_idx;
            int resources[4];
        } recycler;
        struct {
            int selection_idx, scroll_idx, side;
            int confirm;
            int replace_idx;
        } upgrades;
        struct {
            int selection[2];
            int unlocked_ticks;
            int door_idx;
            int num_chars;
            uint8_t chars[8];
        } doorpad;
    } menu;
    bork_entity_t looked_item;
    bork_entity_t looked_enemy;
    struct bork_map_object* looked_obj;
};
/*  Miscellaneous game functions        play_misc.c */
void bork_play_reset_hud_anim(struct bork_play_data* d);
void get_plr_pos_for_ai(struct bork_play_data* d, vec3 out);
void entity_on_fire(struct bork_play_data* d, struct bork_entity* ent);
bork_entity_t get_looked_item(struct bork_play_data* d);
bork_entity_t get_looked_enemy(struct bork_play_data* d);
struct bork_map_object* get_looked_map_object(struct bork_play_data* d);

/*  Cosmetic/mainly visual type stuff   game_effects.c  */
void create_explosion(struct bork_play_data* d, vec3 pos);
void create_smoke(struct bork_play_data* d, vec3 pos, vec3 dir, int lifetime);
void robot_explosion(struct bork_play_data* d, vec3 pos);
void create_spark(struct bork_play_data* d, vec3 pos);
void create_sparks(struct bork_play_data* d, vec3 pos, int sparks);
void tin_canine_tick(struct bork_play_data* d, struct bork_entity* ent);

/*  Recycler menu functions             play_recycler.c */
void tick_recycler_menu(struct bork_play_data* d);
void draw_recycler_menu(struct bork_play_data* d, float t);

/*  Inventory functions                 play_inventory.c     */
void tick_control_inv_menu(struct bork_play_data* d);
void draw_menu_inv(struct bork_play_data* d, float t);
bork_entity_t remove_inventory_item(struct bork_play_data* d, int inv_idx);
void add_inventory_item(struct bork_play_data* d, bork_entity_t ent_id);
void switch_item(struct bork_play_data* d, int inv_idx);
void set_quick_item(struct bork_play_data* d, int quick_idx, int inv_idx);

/*  Upgrade menu functions              play_menu_upgrade.c     */
void tick_upgrades(struct bork_play_data* d);
void tick_control_upgrade_menu(struct bork_play_data* d);
void draw_upgrade_hud(struct bork_play_data* d);
void draw_upgrade_menu(struct bork_play_data* d, float t);
void select_next_upgrade(struct bork_play_data* d);

/*  Door keypad menu functions          play_menu_doorpad.c     */
void tick_doorpad(struct bork_play_data* d);
void draw_doorpad(struct bork_play_data* d, float t);

/*  General UI code */
void draw_quickfetch_text(struct bork_play_data* d, int draw_label,
                          vec4 color_mod, vec4 selected_mod);
void draw_quickfetch_items(struct bork_play_data* d,
                           vec4 color_mod, vec4 selected_mod);

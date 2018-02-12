#include "datapad_content.h"

enum bork_play_tutorial_msg {
    BORK_TUT_PICKUP_ITEM,
    BORK_TUT_VIEW_INVENTORY,
    BORK_TUT_DATAPADS,
    BORK_TUT_CROUCH_FLASHLIGHT,
    BORK_TUT_SCHEMATIC,
    BORK_TUT_RECYCLER,
    BORK_TUT_USE_UPGRADE,
    BORK_TUT_SWITCH_UPGRADE,
    BORK_TUT_SWITCH_AMMO,
    BORK_TUT_DOORS,
    BORK_TUT_MAX
};

#define PLAY_SECONDS(s)     ((int)(s * 120))
struct bork_play_data {
    /*  Core data   */
    int ticks;
    int play_ticks;
    struct bork_game_core* core;
    ARR_T(struct pg_light) lights_buf;    /*  Updated per frame   */
    ARR_T(struct pg_light) spotlights;
    FILE* logfile;
    int log_colls;
    /*  Music and sound stuff   */
    int music_id;
    int music_fade_dir;
    int music_fade_ticks;
    int music_audio_handle;
    int fire_audio_handle;
    int jp_audio_handle;
    /*  Gameplay data   */
    struct bork_map map;
    struct bork_entity plr;
    int plr_map_pos[3];
    int flashlight_on;
    float player_speed;
    int held_item;
    int current_quick_item;
    int quick_item[4];
    int reload_ticks, reload_length;
    int ammo[BORK_AMMO_TYPES];
    int jump_released;
    int teleport_ticks;
    enum bork_upgrade upgrades[4];
    int upgrade_level[4];
    int upgrade_counters[4];
    int upgrade_use_level;
    int upgrade_selected;
    int decoy_active;
    vec3 decoy_pos;
    int held_datapads[NUM_DATAPADS];
    int num_held_datapads;
    uint32_t held_schematics;
    bork_entity_t teleporter_item;
    bork_entity_arr_t plr_enemy_query;
    bork_entity_arr_t plr_item_query;
    bork_entity_arr_t plr_entity_query;
    bork_entity_arr_t inventory;
    bork_entity_arr_t held_upgrades;
    ARR_T(struct bork_bullet) bullets;
    ARR_T(struct bork_particle) particles;
    int killed_laika;
    uint32_t tutorial_msg_seen;
    /*  The HUD-item animation  */
    vec3 hud_anim[5];
    float hud_angle[5];
    float hud_anim_progress;
    float hud_anim_speed;
    int hud_anim_active;
    int hud_anim_destroy_when_finished;
    void (*hud_anim_callback)(struct bork_entity* item, struct bork_play_data* d);
    int hud_callback_frame;
    /*  HUD announcement    */
    int hud_announce_ticks;
    char hud_announce[64];
    /*  HUD datapad info    */
    int hud_datapad_id;
    int hud_datapad_ticks;
    /*  Input states    */
    int draw_ui;
    int draw_weapon;
    int using_jetpack;
    int switch_start_idx;
    int switch_ticks;
    int recoil_time;
    int recoil_ticks;
    vec2 recoil;
    vec2 mouse_motion;
    int crouch_toggle;
    int walk_input_ticks;
    int joystick_held[2];
    int trigger_held[2];
    struct {
        enum {
            BORK_MENU_CLOSED,
            BORK_MENU_INVENTORY,
            BORK_MENU_UPGRADES,
            BORK_MENU_RECYCLER,
            BORK_MENU_DATAPADS,
            BORK_MENU_GAME,
            BORK_MENU_DOORPAD,
            BORK_MENU_PLAYERDEAD,
            BORK_MENU_INTRO,
            BORK_MENU_OUTRO
        } state;
        struct {
            int slide;
            int ticks;
        } intro;
        struct {
            int selection_idx, scroll_idx;
            int ammo_select;
        } inv;
        struct {
            struct bork_map_object* obj;
            int selection_idx, scroll_idx;
            int resources[4];
        } recycler;
        struct {
            int selection_idx, scroll_idx, horiz_idx;
            int confirm, confirm_opt;
            int replace_idx;
        } upgrades;
        struct {
            int selection_idx, scroll_idx, text_scroll;
            int viewing_dp;
        } datapads;
        struct {
            enum { GAME_MENU_BASE, GAME_MENU_LOAD, GAME_MENU_SELECT_SAVE,
                   GAME_MENU_SHOW_OPTIONS, GAME_MENU_EDIT_SAVE,
                   GAME_MENU_CONFIRM_OVERWRITE } mode;
            int save_scroll;
            int save_idx;
            int selection_idx;
            char save_name[32];
            int save_name_len;
            int confirm_opt;
            int opt_idx, opt_scroll, remap_ctrl;
            int opt_fullscreen;
            int opt_res[2];
            char opt_res_input[8];
            int opt_res_input_idx;
            int opt_res_typing;
        } game;
        struct {
            int opt_idx;
        } gameover;
        struct {
            int selection[2];
            int unlocked_ticks;
            int door_idx;
            int num_chars;
            uint8_t chars[8];
        } doorpad;
    } menu;
    char scanned_name[32];
    int scan_ticks;
    bork_entity_t looked_item;
    bork_entity_t looked_enemy;
    bork_entity_t looked_entity;
    struct bork_map_object* looked_obj;
};

/*  Miscellaneous game functions        play_misc.c */
void load_game(struct bork_play_data* d, char* name);
void bork_play_reset_hud_anim(struct bork_play_data* d);
void get_plr_pos_for_ai(struct bork_play_data* d, vec3 out);
void entity_on_fire(struct bork_play_data* d, struct bork_entity* ent);
bork_entity_t get_looked_item(struct bork_play_data* d);
bork_entity_t get_looked_enemy(struct bork_play_data* d);
bork_entity_t get_looked_entity(struct bork_play_data* d);
struct bork_map_object* get_looked_map_object(struct bork_play_data* d);
void game_explosion(struct bork_play_data* d, vec3 pos, float intensity);
void drop_item(struct bork_play_data* d);
void hud_announce(struct bork_play_data* d, char* str);
void hurt_player(struct bork_play_data* d, int damage, int can_block);
void show_tut_message(struct bork_play_data* d, enum bork_play_tutorial_msg msg);

/*  Cosmetic/mainly visual type stuff   game_effects.c  */
void create_explosion(struct bork_play_data* d, vec3 pos, float intensity);
void create_elec_explosion(struct bork_play_data* d, vec3 pos);
void create_smoke(struct bork_play_data* d, vec3 pos, vec3 dir, int lifetime);
void robot_explosion(struct bork_play_data* d, vec3 pos, int num_parts);
void create_spark(struct bork_play_data* d, vec3 pos);
void create_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks);
void create_elec_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks);
void red_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks);
void blue_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks);
void ice_sparks(struct bork_play_data* d, vec3 pos, float expand, int sparks);

/*  Intro functions */
void tick_outro(struct bork_play_data* d);
void draw_outro(struct bork_play_data* d, float t);
void tick_intro(struct bork_play_data* d);
void draw_intro(struct bork_play_data* d, float t);

/*  General menu functions  */
void bork_play_deinit(void* data);
void draw_active_menu(struct bork_play_data* d);

/*  Inventory functions                 play_inventory.c     */
void tick_control_inv_menu(struct bork_play_data* d);
void draw_menu_inv(struct bork_play_data* d, float t);
bork_entity_t remove_inventory_item(struct bork_play_data* d, int inv_idx);
void add_inventory_item(struct bork_play_data* d, bork_entity_t ent_id);
void switch_item(struct bork_play_data* d, int inv_idx);
void next_item(struct bork_play_data* d, int n);
void set_quick_item(struct bork_play_data* d, int quick_idx, int inv_idx);
void draw_quickfetch_text(struct bork_play_data* d, int draw_label,
                          vec4 color_mod, vec4 selected_mod);
void draw_quickfetch_items(struct bork_play_data* d,
                           vec4 color_mod, vec4 selected_mod);

/*  Upgrade menu functions              play_menu_upgrade.c     */
void tick_upgrades(struct bork_play_data* d);
void tick_control_upgrade_menu(struct bork_play_data* d);
void draw_upgrade_hud(struct bork_play_data* d);
void draw_upgrade_menu(struct bork_play_data* d, float t);
void select_next_upgrade(struct bork_play_data* d, int n);

/*  Recycler menu functions             play_recycler.c */
void tick_recycler_menu(struct bork_play_data* d);
void draw_recycler_menu(struct bork_play_data* d, float t);

/*  Datapad menu functions              play_datapads.c */
void tick_datapad_menu(struct bork_play_data* d);
void draw_datapad_menu(struct bork_play_data* d, float t);

/*  Game menu functions                 play_game_menu.c    */
void tick_game_menu(struct bork_play_data* d, struct pg_game_state* state);
void draw_game_menu(struct bork_play_data* d, float t);
void save_game(struct bork_play_data* d, char* filename);
void load_game(struct bork_play_data* d, char* filename);

/*  Door keypad menu functions          play_menu_doorpad.c     */
void tick_doorpad(struct bork_play_data* d);
void draw_doorpad(struct bork_play_data* d, float t);
#include "tinyfiles.h"

enum bork_control {
    BORK_CTRL_UP,
    BORK_CTRL_DOWN,
    BORK_CTRL_LEFT,
    BORK_CTRL_RIGHT,
    BORK_CTRL_JUMP,
    BORK_CTRL_CROUCH,
    BORK_CTRL_FLASHLIGHT,
    BORK_CTRL_FIRE,
    BORK_CTRL_RELOAD,
    BORK_CTRL_DROP,
    BORK_CTRL_INTERACT,
    BORK_CTRL_USE_TECH,
    BORK_CTRL_NEXT_TECH,
    BORK_CTRL_PREV_TECH,
    BORK_CTRL_NEXT_ITEM,
    BORK_CTRL_PREV_ITEM,
    BORK_CTRL_BIND1,
    BORK_CTRL_BIND2,
    BORK_CTRL_BIND3,
    BORK_CTRL_BIND4,
    BORK_CTRL_MENU,
    BORK_CTRL_MENU_BACK,
    BORK_CTRL_SELECT,
    BORK_CTRL_COUNT,
};

enum bork_sound {
    BORK_SND_PISTOL,
    BORK_SND_SHOTGUN,
    BORK_SND_MACHINEGUN,
    BORK_SND_PLAZGUN,
    BORK_SND_BULLET_HIT,
    BORK_SND_PLAZMA_HIT,
    BORK_SND_EXPLOSION,
    BORK_SND_EXPLOSION_ELEC,
    BORK_SND_TELEPORT,
    BORK_SND_PICKUP,
    BORK_SND_RECYCLER,
    BORK_SND_DEFENSE_FIELD,
    BORK_SND_DOOR_OPEN,
    BORK_SND_DOOR_CLOSE,
    BORK_SND_FIRE,
    BORK_SND_FOOTSTEP1,
    BORK_SND_FOOTSTEP2,
    BORK_SND_JUMP,
    BORK_SND_PLAYER_LAND,
    BORK_SND_ITEM_LAND,
    BORK_SND_KEYPAD_PRESS,
    BORK_SND_SWING_PIPE,
    BORK_SND_SWING_BEAMSWORD,
    BORK_SND_CHARGE,
    BORK_SND_FASTBEEPS,
    BORK_SND_SINGLEBEEP,
    BORK_SND_HURT,
    BORK_SND_HACK,
    BORK_SND_HEAL_TECH,
    BORK_SND_RELOAD_START,
    BORK_SND_RELOAD_END,
    BORK_SND_HUM,
    BORK_SND_HISS,
    BORK_SND_COMPUTERS,
    BORK_SND_BUZZ,
    BORK_SND_HUM2,
    BORK_SND_HUM3,
    BORK_MUS_MAINMENU,
    BORK_MUS_BOSSFIGHT,
    BORK_MUS_ENDGAME,
    BORK_NUM_SOUNDS
};

enum bork_option {
    BORK_OPT_FULLSCREEN,
    BORK_OPT_RES_X,
    BORK_OPT_RES_Y,
    BORK_OPT_SHOW_FPS,
    BORK_OPT_GAMMA,
    BORK_OPT_MUSIC_VOL,
    BORK_OPT_SFX_VOL,
    BORK_OPT_INVERT_Y,
    BORK_OPT_MOUSE_SENS,
    BORK_OPT_GAMEPAD,
    BORK_OPTS
};

#define BORK_FULL_OPTS      (BORK_OPTS + BORK_CTRL_COUNT + 1)

struct bork_save {
    char name[32];
    tfFILETIME time;
};

struct bork_game_core {
    char* base_path;
    int base_path_len;
    int free_base_path;
    int user_exit;
    /*  Rendering data  */
    float gamma;
    int fullscreen;
    vec2 screen_size;
    float aspect_ratio;
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    struct pg_shader shader_3d;
    struct pg_shader shader_2d;
    struct pg_shader shader_sprite;
    struct pg_shader shader_text;
    struct pg_ppbuffer ppbuf;
    struct pg_postproc post_blur;
    struct pg_postproc post_gamma;
    struct pg_postproc post_screen;
    /*  Assets  */
    struct pg_texture env_atlas;
    struct pg_texture editor_atlas;
    struct pg_texture bullet_tex;
    struct pg_texture particle_tex;
    struct pg_texture upgrades_tex;
    struct pg_texture item_tex;
    struct pg_texture enemies_tex;
    struct pg_texture starfield_tex;
    struct pg_texture moon_tex;
    struct pg_texture font;
    struct pg_audio_chunk sounds[BORK_NUM_SOUNDS];
    /*  Generated assets    */
    struct pg_texture backdrop_tex;
    struct pg_texture menu_vignette;
    struct pg_texture radial_vignette;
    struct pg_model quad_2d;
    struct pg_model quad_2d_ctr;
    struct pg_model bullet_model;
    struct pg_model enemy_model;
    struct pg_model gun_model;
    struct pg_audio_chunk menu_sound;
    /*  Input state */
    float music_volume, sfx_volume;
    int invert_y;
    int show_fps;
    uint8_t ctrl_map[BORK_CTRL_COUNT];
    int8_t gpad_map[BORK_CTRL_COUNT];
    float mouse_sensitivity;
    float joy_sensitivity;
    int mouse_relative;
    int gpad_idx;
    /*  Save files  */
    ARR_T(struct bork_save) save_files;
};

struct bork_map;
struct bork_play_data;

void bork_init(struct bork_game_core* core, char* base_path);
void bork_set_gamma(struct bork_game_core* core, float gamma);
void bork_set_music_volume(struct bork_game_core* core, float vol);
void bork_set_sfx_volume(struct bork_game_core* core, float vol);
void bork_read_saves(struct bork_game_core* core);
void bork_delete_save(struct bork_game_core* core, int save_idx);
void bork_load_options(struct bork_game_core* core);
void bork_save_options(struct bork_game_core* core);
void bork_reinit_gfx(struct bork_game_core* core, int sw, int sh, int fullscreen);
void bork_reset_keymap(struct bork_game_core* core);
void bork_reset_gamepad_map(struct bork_game_core* core);
const char* bork_get_ctrl_name(enum bork_control ctrl);
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);

void bork_draw_fps(struct bork_game_core* core);
void bork_draw_backdrop(struct bork_game_core* core, vec4 color_mod, float t);
void bork_draw_linear_vignette(struct bork_game_core* core, vec4 color_mod);

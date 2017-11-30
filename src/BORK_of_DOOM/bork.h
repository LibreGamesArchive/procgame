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
    BORK_SND_HURT,
    BORK_SND_HUM,
    BORK_SND_HISS,
    BORK_SND_BEEPS,
    BORK_SND_BUZZ,
    BORK_NUM_SOUNDS
};

struct bork_save {
    char name[32];
};

struct bork_game_core {
    char* base_path;
    int base_path_len;
    int free_base_path;
    int user_exit;
    /*  Rendering data  */
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
    int show_fps;
    uint8_t ctrl_map[BORK_CTRL_COUNT];
    float mouse_sensitivity;
    int mouse_relative;
    SDL_GameController* gpad;
    /*  Save files  */
    ARR_T(struct bork_save) save_files;
};

struct bork_map;
struct bork_play_data;

void bork_init(struct bork_game_core* core, char* base_path);
void bork_delete_save(struct bork_game_core* core, int save_idx);
void bork_load_options(struct bork_game_core* core);
void bork_save_options(struct bork_game_core* core);
void bork_reinit_gfx(struct bork_game_core* core, int sw, int sh, int fullscreen);
void bork_reset_keymap(struct bork_game_core* core);
const char* bork_get_ctrl_name(enum bork_control ctrl);
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);

void bork_draw_fps(struct bork_game_core* core);
void bork_draw_backdrop(struct bork_game_core* core, vec4 color_mod, float t);
void bork_draw_linear_vignette(struct bork_game_core* core, vec4 color_mod);

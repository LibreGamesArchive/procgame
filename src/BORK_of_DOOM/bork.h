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

static const char* bork_ctrl_names[] = {
    [BORK_CTRL_UP] =            "WALK FORWARD",
    [BORK_CTRL_DOWN] =          "WALK BACKWARD",
    [BORK_CTRL_LEFT] =          "WALK LEFT",
    [BORK_CTRL_RIGHT] =         "WALK RIGHT",
    [BORK_CTRL_JUMP] =          "JUMP",
    [BORK_CTRL_CROUCH] =        "CROUCH",
    [BORK_CTRL_FLASHLIGHT] =    "FLASHLIGHT",
    [BORK_CTRL_FIRE] =          "USE HELD ITEM (SHOOT)",
    [BORK_CTRL_RELOAD] =        "RELOAD",
    [BORK_CTRL_DROP] =          "DROP ITEM",
    [BORK_CTRL_INTERACT] =      "INTERACT",
    [BORK_CTRL_USE_TECH] =      "USE TECH UPGRADE",
    [BORK_CTRL_NEXT_TECH] =     "NEXT UPGRADE",
    [BORK_CTRL_PREV_TECH] =     "PREVIOUS UPGRADE",
    [BORK_CTRL_NEXT_ITEM] =     "NEXT ITEM",
    [BORK_CTRL_PREV_ITEM] =     "PREV ITEM",
    [BORK_CTRL_BIND1] =         "QUICK FETCH 1",
    [BORK_CTRL_BIND2] =         "QUICK FETCH 2",
    [BORK_CTRL_BIND3] =         "QUICK FETCH 3",
    [BORK_CTRL_BIND4] =         "QUICK FETCH 4",
    [BORK_CTRL_MENU] =          "MENU",
    [BORK_CTRL_MENU_BACK] =     "MENU BACK",
    [BORK_CTRL_SELECT] =        "SELECT OPTION",
    [BORK_CTRL_COUNT] =         "NULL"
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
    struct pg_texture font;
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
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);

void bork_draw_fps(struct bork_game_core* core);
void bork_draw_backdrop(struct bork_game_core* core, vec4 color_mod, float t);
void bork_draw_linear_vignette(struct bork_game_core* core, vec4 color_mod);

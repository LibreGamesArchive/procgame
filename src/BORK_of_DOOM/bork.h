enum bork_control {
    BORK_CTRL_UP,
    BORK_CTRL_DOWN,
    BORK_CTRL_LEFT,
    BORK_CTRL_RIGHT,
    BORK_CTRL_JUMP,
    BORK_CTRL_FIRE,
    BORK_CTRL_SELECT,
    BORK_CTRL_ESCAPE,
    BORK_CTRL_BIND1,
    BORK_CTRL_BIND2,
    BORK_CTRL_BIND3,
    BORK_CTRL_BIND4,
    BORK_CTRL_NULL,
};

#define BORK_CONTROL_HIT        1
#define BORK_CONTROL_HELD       2
#define BORK_CONTROL_RELEASED   3

struct bork_game_core {
    int user_exit;
    /*  Rendering data  */
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
    /*  Assets  */
    struct pg_texture env_atlas;
    struct pg_texture editor_atlas;
    struct pg_texture bullet_tex;
    struct pg_texture item_tex;
    struct pg_texture font;
    /*  Generated assets    */
    struct pg_texture backdrop_tex;
    struct pg_texture radial_vignette_tex;
    struct pg_model quad_2d;
    struct pg_model bullet_model;
    struct pg_model enemy_model;
    struct pg_model gun_model;
    /*  Input state */
    uint8_t ctrl_state[BORK_CTRL_NULL];
};

void bork_init(struct bork_game_core* core);
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_poll_input(struct bork_game_core* core);
void bork_update_inputs(struct bork_game_core* core);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);

void bork_draw_fps(struct bork_game_core* core);
void bork_draw_backdrop(struct bork_game_core* core, vec4 color_mod, float t);

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

#define BORK_CONTROL_RELEASED   1
#define BORK_CONTROL_HIT        2
#define BORK_CONTROL_HELD       3
/*  Special mouse control codes, mapped to unused SDL scancodes */
#define BORK_LEFT_MOUSE         253
#define BORK_RIGHT_MOUSE        254
#define BORK_MIDDLE_MOUSE       255

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
    struct pg_texture menu_vignette;
    struct pg_model quad_2d;
    struct pg_model bullet_model;
    struct pg_model enemy_model;
    struct pg_model gun_model;
    struct pg_audio_chunk menu_sound;
    /*  Input state */
    uint8_t ctrl_map[BORK_CTRL_NULL];
    uint8_t ctrl_state[256];
    uint8_t ctrl_changes[16];
    uint8_t ctrl_changed;
    vec2 mouse_pos;
    vec2 mouse_motion;
    float mouse_sensitivity;
    int mouse_relative;
};

void bork_init(struct bork_game_core* core);
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_poll_input(struct bork_game_core* core);
int bork_input_event(struct bork_game_core* core, uint8_t ctrl, uint8_t event);
void bork_ack_input(struct bork_game_core* core);
void bork_grab_mouse(struct bork_game_core* core, int grab);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);

void bork_draw_fps(struct bork_game_core* core);
void bork_draw_backdrop(struct bork_game_core* core, vec4 color_mod, float t);
void bork_draw_linear_vignette(struct bork_game_core* core, vec4 color_mod);

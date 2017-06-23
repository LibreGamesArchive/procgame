enum bork_direction {
    BORK_FRONT = 0,
    BORK_Y_POS = BORK_FRONT,
    BORK_BACK = 1,
    BORK_Y_NEG = BORK_BACK,
    BORK_LEFT = 2,
    BORK_X_POS = BORK_LEFT,
    BORK_RIGHT = 3,
    BORK_X_NEG = BORK_RIGHT,
    BORK_UP = 4,
    BORK_Z_POS = BORK_UP,
    BORK_TOP = BORK_UP,
    BORK_DOWN = 5,
    BORK_Z_NEG = BORK_DOWN,
    BORK_BOTTOM = BORK_DOWN
};
#define BORK_DIR_OPPOSITE(a)    (a % 2 ? a - 1 : a + 1)

struct bork_game_core {
    /*  Rendering data  */
    int screen_size[2];
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    struct pg_shader shader_3d;
    struct pg_shader shader_text;
    /*  Assets  */
    struct pg_model map_model;
    struct pg_texture env_atlas;
    struct pg_texture font;
};

void bork_init(struct bork_game_core* core);
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);

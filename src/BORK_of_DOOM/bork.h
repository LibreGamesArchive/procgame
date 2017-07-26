struct bork_light {
    vec4 pos;
    vec3 color;
};

struct bork_game_core {
    /*  Rendering data  */
    int screen_size[2];
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    ARR_T(struct bork_light) lights_buf;    /*  Updated per frame   */
    struct pg_shader shader_3d;
    struct pg_shader shader_2d;
    struct pg_shader shader_sprite;
    struct pg_shader shader_text;
    /*  Assets  */
    struct pg_texture env_atlas;
    struct pg_texture editor_atlas;
    struct pg_texture bullet_tex;
    struct pg_texture font;
    struct pg_model bullet_model;
    struct pg_model enemy_model;
    struct bork_entity* plr;
};

void bork_init(struct bork_game_core* core);
void bork_deinit(struct bork_game_core* core);
void bork_load_assets(struct bork_game_core* core);
void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_play_start(struct pg_game_state* state, struct bork_game_core* core);
void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core);

enum bork_area_id {
    BORK_AREA_COMMAND_STATION,
    BORK_AREA_OFFICES,
    BORK_AREA_WAREHOUSE,
    BORK_AREA_UNION_HALL,
    BORK_AREA_INFIRMARY,
    BORK_AREA_RECREATION,
    BORK_AREA_SCIENCE_LABS,
    BORK_AREA_CAFETERIA,
    BORK_AREA_KITCHEN,
    BORK_AREA_PETS,
    BORK_AREA_EXTERIOR,
    BORK_AREA_MUTT,
};

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

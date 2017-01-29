struct pg_terrain {
    struct pg_model model;
    float* heightmap;
    float precision;
    int w, h;
};

void pg_terrain_init(struct pg_terrain* terr, int w, int h,
                         float precision);
void pg_terrain_deinit(struct pg_terrain* terr);
void pg_terrain_buffer(struct pg_terrain* terr, struct pg_renderer* rend);
void pg_terrain_begin(struct pg_terrain* terr,
                          struct pg_renderer* rend);
void pg_terrain_update(struct pg_terrain* terr);
void pg_terrain_draw(struct pg_renderer* rend,
                         struct pg_terrain* terr, mat4 transform);

void pg_terrain_from_wave(struct pg_terrain* terr, struct pg_wave* wave,
                              vec3 domain);
void pg_terrain_perlin(struct pg_terrain* terr, float scale,
                           float x1, float y1, float x2, float y2);
void pg_terrain_normal(struct pg_terrain* terr, int x, int y, vec3 out);
float pg_terrain_height(struct pg_terrain* terr, int x, int y);

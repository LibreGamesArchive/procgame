struct procgl_terrain {
    struct procgl_model model;
    float* heightmap;
    float precision;
    int w, h;
};

void procgl_terrain_init(struct procgl_terrain* terr, int w, int h,
                         float precision);
void procgl_terrain_deinit(struct procgl_terrain* terr);
void procgl_terrain_buffer(struct procgl_terrain* terr);
void procgl_terrain_begin(struct procgl_terrain* terr,
                          struct procgl_renderer* rend);
void procgl_terrain_update(struct procgl_terrain* terr);
void procgl_terrain_draw(struct procgl_renderer* rend,
                         struct procgl_terrain* terr, mat4 transform);

void procgl_terrain_from_wave(struct procgl_terrain* terr, struct procgl_wave* wave,
                              vec3 domain);
void procgl_terrain_perlin(struct procgl_terrain* terr, float scale,
                           float x1, float y1, float x2, float y2);
void procgl_terrain_normal(struct procgl_terrain* terr, int x, int y, vec3 out);
float procgl_terrain_height(struct procgl_terrain* terr, int x, int y);

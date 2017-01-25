struct procgl_texture_pixel {
    unsigned char r, g, b, a;
};

struct procgl_texture_normal {
    unsigned char x, y, z, h;
};

struct procgl_texture {
    struct procgl_texture_pixel* pixels;
    struct procgl_texture_normal* normals;
    int w, h;
    GLuint pixels_gl;
    GLuint normals_gl;
};

void procgl_texture_init(struct procgl_texture* tex, int w, int h);
void procgl_texture_deinit(struct procgl_texture* tex);
void procgl_texture_buffer(struct procgl_texture* tex);
void procgl_texture_use(struct procgl_texture* tex,
                        struct procgl_renderer* rend, int slot);
void procgl_texture_use_terrain(struct procgl_texture* tex,
                                struct procgl_renderer* rend, int slot,
                                float height_mod, float scale,
                                float detail_weight);
void procgl_texture_perlin(struct procgl_texture* tex,
                           float x1, float y1, float x2, float y2);
void procgl_texture_shitty(struct procgl_texture* tex);
void procgl_texture_generate_normals(struct procgl_texture* tex);

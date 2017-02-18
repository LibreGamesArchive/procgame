struct pg_texture_pixel {
    unsigned char r, g, b, a;
};

struct pg_texture_normal {
    unsigned char x, y, z, h;
};

struct pg_texture {
    struct pg_texture_pixel* pixels;
    struct pg_texture_normal* normals;
    int w, h;
    GLuint pixels_gl;
    GLuint normals_gl;
};

void pg_texture_init(struct pg_texture* tex, int w, int h);
void pg_texture_deinit(struct pg_texture* tex);
void pg_texture_buffer(struct pg_texture* tex);
void pg_texture_perlin(struct pg_texture* tex,
                           float x1, float y1, float x2, float y2);
void pg_texture_shitty(struct pg_texture* tex);
void pg_texture_generate_normals(struct pg_texture* tex);

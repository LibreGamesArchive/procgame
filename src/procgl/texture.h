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
    int frame_h, frame_w;
    int color_slot, normal_slot;
    GLuint pixels_gl;
    GLuint normals_gl;
};

void pg_texture_init_from_file(struct pg_texture* tex,
                               const char* color_file, const char* normal_file,
                               int color_slot, int normal_slot);
void pg_texture_init(struct pg_texture* tex, int w, int h,
                     int color_slot, int normal_slot);
void pg_texture_deinit(struct pg_texture* tex);
void pg_texture_bind(struct pg_texture* tex, int color_slot, int normal_slot);
void pg_texture_buffer(struct pg_texture* tex);
void pg_texture_perlin(struct pg_texture* tex,
                           float x1, float y1, float x2, float y2);
void pg_texture_shitty(struct pg_texture* tex);
void pg_texture_generate_normals(struct pg_texture* tex, float intensity);
void pg_texture_set_atlas(struct pg_texture* tex, int frame_w, int frame_h);
void pg_texture_get_frame(struct pg_texture* tex, int frame,
                          vec2 start, vec2 end);

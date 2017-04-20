typedef unsigned char pg_texel_t[4];

static inline void pg_texel_set(pg_texel_t t, unsigned char r, unsigned char g,
                                unsigned char b, unsigned char a)
{
    t[0] = r; t[1] = g; t[2] = b; t[3] = a;
}

struct pg_texture {
    pg_texel_t* diffuse;
    pg_texel_t* light;  /*  RGB for normal, A for specular  */
    GLuint diffuse_gl;
    GLuint light_gl;
    int diffuse_slot, light_slot;
    int w, h;
    int frame_h, frame_w;
};

struct pg_texture_cube {
    struct pg_texture* tex[6];
};

void pg_texture_init_from_file(struct pg_texture* tex, const char* diffuse_file,
                               const char* light_file);
void pg_texture_init(struct pg_texture* tex, int w, int h);
void pg_texture_deinit(struct pg_texture* tex);
void pg_texture_bind(struct pg_texture* tex, int diffuse_slot, int light_slot);
void pg_texture_generate_normals(struct pg_texture* tex,
                                 struct pg_heightmap* hmap, float intensity);
void pg_texture_generate_mipmaps(struct pg_texture* tex);
void pg_texture_buffer(struct pg_texture* tex);
void pg_texture_set_atlas(struct pg_texture* tex, int frame_w, int frame_h);
void pg_texture_get_frame(struct pg_texture* tex, int frame,
                          vec2 start, vec2 end);

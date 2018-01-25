struct pg_texture {
    void* data;
    enum pg_data_type type;
    int channels;
    GLuint handle;
    int w, h;
    int frame_h, frame_w;
    float frame_aspect_ratio;
};

/*  Load a texture from a file, to a 4-channel 32bpp RGBA texture   */
void pg_texture_init_from_file(struct pg_texture* tex, const char* file);
/*  Initialize an empty texture */
void pg_texture_init(struct pg_texture* tex, int w, int h,
                     enum pg_data_type type);
/*  Free a texture initialized with above functions */
void pg_texture_deinit(struct pg_texture* tex);

/*  Bind texture to texture unit GL_TEXTURE0+slot   */
void pg_texture_bind(struct pg_texture* tex, int slot);
/*  Upload texture data to the GPU  */
void pg_texture_buffer(struct pg_texture* tex);
/*  OpenGL generate mipmaps */
void pg_texture_generate_mipmaps(struct pg_texture* tex);

/*  Sets the dimensions of a texture atlas grid */
void pg_texture_set_atlas(struct pg_texture* tex, int frame_w, int frame_h);
/*  Retrieves a vec4 texture transform for a given frame (x, y, w, h)   */
void pg_texture_frame(struct pg_texture* tex, int frame, vec4 out);
/*  Flip a texture transform on given axes  */
void pg_texture_frame_flip(vec4 out, vec4 const in, int x, int y);
/*  Intuitively transform a texture transform: (x,y) + offset, (w,h) * scale */
void pg_texture_frame_tx(vec4 out, vec4 const in,
                         vec2 const scale, vec2 const offset);

struct pg_renderbuffer {
    GLuint fbo;
    struct pg_texture* outputs[8];
    GLenum attachments[8];
    struct pg_texture* depth;
    GLenum depth_attachment;
    int dirty, w, h;
};

struct pg_rendertarget {
    struct pg_renderbuffer* buf[2];
    int cur_dst;
};

/*  Rendertexture   */
void pg_renderbuffer_init(struct pg_renderbuffer* buffer);
void pg_renderbuffer_attach(struct pg_renderbuffer* buffer,
                             struct pg_texture* tex,
                             int idx, GLenum attachment);
void pg_renderbuffer_dst(struct pg_renderbuffer* buffer);
void pg_rendertarget_init(struct pg_rendertarget* target,
                          struct pg_renderbuffer* b0, struct pg_renderbuffer* b1);
void pg_rendertarget_dst(struct pg_rendertarget* target);
void pg_rendertarget_swap(struct pg_rendertarget* target);











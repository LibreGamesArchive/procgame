struct pg_ppbuffer {
    GLuint color[2];
    GLuint depth[2];
    GLuint frame[2];
    GLenum color_unit[2];
    GLenum depth_unit[2];
    int dst;
    int w, h;
};

struct pg_postproc {
    GLuint vert, frag, prog;
    GLuint vert_buf, tri_buf, vao;
    GLint tex_unit, depth_unit;
    GLint attrib_pos;
    void* data;
    void (*deinit)(void*);
    void (*pre)(struct pg_postproc*);
};

void pg_ppbuffer_init(struct pg_ppbuffer* buf, int w, int h,
                      int color0, int color1, int depth0, int depth1);
void pg_ppbuffer_deinit(struct pg_ppbuffer* buf);
void pg_ppbuffer_set_units(struct pg_ppbuffer* buf,
                           GLenum color0, GLenum color1,
                           GLenum depth0, GLenum depth1);
void pg_ppbuffer_bind_all(struct pg_ppbuffer* buf);
void pg_ppbuffer_bind_active(struct pg_ppbuffer* buf);
void pg_ppbuffer_dst(struct pg_ppbuffer* buf);
void pg_ppbuffer_swap(struct pg_ppbuffer* buf);

void pg_postproc_load(struct pg_postproc* pp, const char* vert_filename,
                      const char* frag_filename, const char* pos_attrib,
                      const char* color_name, const char* depth_name);
void pg_postproc_deinit(struct pg_postproc* pp);
void pg_postproc_apply(struct pg_postproc* pp, struct pg_ppbuffer* src,
                       struct pg_ppbuffer* dst);

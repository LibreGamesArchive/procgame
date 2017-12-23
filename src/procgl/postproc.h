struct pg_ppbuffer {
    GLuint color[2];
    GLuint frame[2];
    int color_unit[2];
    int dst;
    int w, h;
};

struct pg_postproc {
    GLuint vert, frag, prog, dummy_vao;
    GLint tex_unit, uni_size;
    void* data;
    void (*deinit)(void*);
    void (*pre)(struct pg_postproc*);
};

void pg_ppbuffer_init(struct pg_ppbuffer* buf, int w, int h,
                      int color0, int color1);
void pg_ppbuffer_deinit(struct pg_ppbuffer* buf);
void pg_ppbuffer_set_units(struct pg_ppbuffer* buf,
                           GLenum color0, GLenum color1);
void pg_ppbuffer_bind_all(struct pg_ppbuffer* buf);
void pg_ppbuffer_bind_active(struct pg_ppbuffer* buf);
void pg_ppbuffer_dst(struct pg_ppbuffer* buf);
void pg_ppbuffer_swap(struct pg_ppbuffer* buf);
void pg_ppbuffer_swapdst(struct pg_ppbuffer* buf);

void pg_postproc_load(struct pg_postproc* pp,
                      const char* vert_filename, const char* frag_filename,
                      const char* color_name, const char* size_name);
void pg_postproc_load_static(struct pg_postproc* pp,
                      const char* vert, int vert_len,
                      const char* frag, int frag_len,
                      const char* color_name, const char* size_name);
void pg_postproc_deinit(struct pg_postproc* pp);
void pg_postproc_apply(struct pg_postproc* pp, struct pg_ppbuffer* src);

/*  The basic no-post-process post-process  */
void pg_postproc_screen(struct pg_postproc* pp);

void pg_postproc_gamma(struct pg_postproc* pp);
void pg_postproc_gamma_set(struct pg_postproc* pp, float gamma);

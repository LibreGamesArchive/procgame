struct pg_gbuffer {
    /*  The texture units that all the buffers should be kept in    */
    int color_slot, normal_slot, depth_slot, light_slot;
    /*  The shader program for rendering light volumes  */
    GLuint l_vert, l_frag, l_prog;
    GLuint dummy_vao;
    GLint uni_projview, uni_view, uni_eye_pos, uni_clip;
    GLint uni_normal, uni_depth, uni_light, uni_color;
    /*  The shader program for rendering the final image    */
    GLuint f_vert, f_frag, f_prog;
    GLint f_color, f_light, f_ambient;
    /*  The G-buffer, renderbuffer for depth, and light accumulation buffer */
    GLuint color;
    GLuint normal;
    GLuint depth;
    GLuint light;
    GLuint frame;
    int w, h;
};

/*  Texture slot 0 is used for initialization, make sure to bind the buffer to
    the desired slots afterward */
void pg_gbuffer_init(struct pg_gbuffer* gbuf, int w, int h);
void pg_gbuffer_deinit(struct pg_gbuffer* gbuf);
/*  All buffer slots must be bound  */
void pg_gbuffer_bind(struct pg_gbuffer* gbuf, int color_slot,
                     int normal_slot, int pos_slot, int light_slot);
/*  This begins drawing to the G-buffer. Make sure to only use shaders which
    are written to output to the first three color attachments,
    (0: color buffer, 1: world-space normals, 2: world-space positions) */
void pg_gbuffer_dst(struct pg_gbuffer* gbuf);
/*  These three perform the lighting pass. After all regular drawing is done,
    call pg_gbuffer_begin_light(), then draw_light() for every light in the
    scene, and finish() to draw the final result to the given ppbuffer, or
    to the screen directly if ppbuf is set to NULL  */
void pg_gbuffer_begin_light(struct pg_gbuffer* gbuf, struct pg_viewer* view);
void pg_gbuffer_draw_light(struct pg_gbuffer* gbuf, vec4 light, vec3 color);
void pg_gbuffer_finish(struct pg_gbuffer* gbuf, vec3 ambient_light);

struct pg_light {
    vec3 pos, color;
    float size, angle;
    vec4 dir_quat;
};

struct pg_gbuffer {
    /*  The texture units that all the buffers should be kept in    */
    int color_slot, normal_slot, depth_slot, light_slot;
    GLuint dummy_vao;
    /*  The shader program for rendering light volumes  (point lights)  */
    GLuint l_vert, l_frag, l_prog;
    GLint uni_projview, uni_view, uni_eye_pos, uni_clip;
    GLint uni_normal, uni_depth, uni_light, uni_color;
    /*  The shader program for rendering light volumes  (spot lights)  */
    GLuint spot_vert, spot_frag, spot_prog;
    GLint uni_projview_spot, uni_view_spot, uni_eye_pos_spot, uni_clip_spot;
    GLint uni_normal_spot, uni_depth_spot, uni_light_spot, uni_dir_spot, uni_color_spot;
    /*  The shader program for rendering the final image    */
    GLuint f_vert, f_frag, f_prog;
    GLint f_color, f_light, f_norm, f_ambient;
    /*  The G-buffer, renderbuffer for depth, and light accumulation buffer */
    GLuint color;
    GLuint normal;
    GLuint depth;
    GLuint light;
    GLuint frame;
    int w, h;
};

void pg_light_spotlight(struct pg_light* light, vec3 const pos, float size,
                        vec3 const color, vec3 const dir, float angle);
void pg_light_pointlight(struct pg_light* light, vec3 const pos, float size,
                         vec3 const color);

/*  Texture slot 0 is used for initialization, make sure to bind the buffer to
    the desired slots afterward */
void pg_gbuffer_init(struct pg_gbuffer* gbuf, int w, int h);
void pg_gbuffer_deinit(struct pg_gbuffer* gbuf);
/*  All buffer slots must be bound  */
void pg_gbuffer_bind(struct pg_gbuffer* gbuf, int color_slot,
                     int normal_slot, int depth_slot, int light_slot);
/*  This begins drawing to the G-buffer. Make sure to only use shaders which
    are written to output to the first three color attachments,
    (0: color buffer, 1: world-space normals, 2: world-space positions) */
void pg_gbuffer_dst(struct pg_gbuffer* gbuf);
void pg_gbuffer_begin_pointlight(struct pg_gbuffer* gbuf, struct pg_viewer* view);
void pg_gbuffer_begin_spotlight(struct pg_gbuffer* gbuf, struct pg_viewer* view);
void pg_gbuffer_draw_pointlight(struct pg_gbuffer* gbuf, struct pg_light* light);
void pg_gbuffer_draw_spotlight(struct pg_gbuffer* gbuf, struct pg_light* light);
/*  Set mode to 1 to draw lights when the viewer is inside the light volume,
    and 0 otherwise. Light drawing begins in mode 0 with each "begin"
    function call.  */
void pg_gbuffer_mode(struct pg_gbuffer* gbuf, int mode);
void pg_gbuffer_finish(struct pg_gbuffer* gbuf, vec3 ambient_light);

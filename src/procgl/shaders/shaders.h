struct pg_shader;
struct pg_texture;

/*  My shader implementations   */
struct pg_shader_text {
    char block[32][64];
    vec4 block_style[32];
    vec4 block_color[32];
    int use_blocks;
};

int pg_shader_text(struct pg_shader* shader);
void pg_shader_text_3d(struct pg_shader* shader, struct pg_viewer* view);
void pg_shader_text_resolution(struct pg_shader* shader, vec2 const resolution);
void pg_shader_text_ndc(struct pg_shader* shader, vec2 const scale);
void pg_shader_text_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_text_transform_3d(struct pg_shader* shader, mat4 tx);
void pg_shader_text_font(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_text_write(struct pg_shader* shader, struct pg_shader_text* text);

#define PG_SPRITE_SPHERICAL     0
#define PG_SPRITE_CYLINDRICAL   1
int pg_shader_sprite(struct pg_shader* shader);
void pg_shader_sprite_mode(struct pg_shader* shader, int mode);
void pg_shader_sprite_texture(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_sprite_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_sprite_tex_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_sprite_add_tex_tx(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_sprite_tex_frame(struct pg_shader* shader, int frame);
void pg_shader_sprite_color_mod(struct pg_shader* shader, vec4 const color_mod);

int pg_shader_2d(struct pg_shader* shader);
void pg_shader_2d_resolution(struct pg_shader* shader, vec2 const resolution);
void pg_shader_2d_ndc(struct pg_shader* shader, vec2 const scale);
void pg_shader_2d_transform(struct pg_shader* shader, vec2 const pos, vec2 const size,
                            float rotation);
void pg_shader_2d_tex_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_2d_add_tex_tx(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_2d_texture(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_2d_tex_weight(struct pg_shader* shader, float weight);
void pg_shader_2d_tex_frame(struct pg_shader* shader, int frame);
void pg_shader_2d_color_mod(struct pg_shader* shader, vec4 const color, vec4 const add);
void pg_shader_2d_set_light(struct pg_shader* shader, vec2 const pos,
                            vec3 const color, vec3 const ambient);

int pg_shader_3d(struct pg_shader* shader);
void pg_shader_3d_texture(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_3d_tex_frame(struct pg_shader* shader, int frame);
void pg_shader_3d_tex_transform(struct pg_shader* shader, vec2 const scale, vec2 const offset);
void pg_shader_3d_add_tex_tx(struct pg_shader* shader, vec2 const scale, vec2 const offset);

int pg_shader_cubetex(struct pg_shader* shader);
void pg_shader_cubetex_set_texture(struct pg_shader* shader,
                                   struct pg_texture_cube* tex);
void pg_shader_cubetex_blend_sharpness(struct pg_shader* shader, float k);

struct pg_postproc;
enum pg_postproc_blur_level{ PG_BLUR3, PG_BLUR5, PG_BLUR7 };
void pg_postproc_blur(struct pg_postproc* pp, enum pg_postproc_blur_level level);
void pg_postproc_blur_dir(struct pg_postproc* pp, int dir);
void pg_postproc_blur_scale(struct pg_postproc* pp, vec2 const scale);

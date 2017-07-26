/*  My shader implementations   */
int pg_shader_text(struct pg_shader* shader);
void pg_shader_text_set_font(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_text_write(struct pg_shader* shader, const char* str,
                          vec2 start, vec2 size, float space);

#define PG_SPRITE_SPHERICAL     0
#define PG_SPRITE_CYLINDRICAL   1
int pg_shader_sprite(struct pg_shader* shader);
void pg_shader_sprite_set_mode(struct pg_shader* shader, int mode);
void pg_shader_sprite_set_texture(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_sprite_set_tex_frame(struct pg_shader* shader, int frame);

int pg_shader_2d(struct pg_shader* shader);
void pg_shader_2d_set_texture(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_2d_set_tex_weight(struct pg_shader* shader, float weight);
void pg_shader_2d_set_tex_offset(struct pg_shader* shader, vec2 offset);
void pg_shader_2d_set_tex_scale(struct pg_shader* shader, vec2 scale);
void pg_shader_2d_set_tex_frame(struct pg_shader* shader, int frame);
void pg_shader_2d_set_color_mod(struct pg_shader* shader, vec4 color);

int pg_shader_3d(struct pg_shader* shader);
void pg_shader_3d_set_texture(struct pg_shader* shader, struct pg_texture* tex);

int pg_shader_cubetex(struct pg_shader* shader);
void pg_shader_cubetex_set_texture(struct pg_shader* shader,
                                   struct pg_texture_cube* tex);
void pg_shader_cubetex_blend_sharpness(struct pg_shader* shader, float k);

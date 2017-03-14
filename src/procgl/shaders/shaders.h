/*  My shader implementations   */
int pg_shader_text(struct pg_shader* shader);
void pg_shader_text_set_font(struct pg_shader* shader, struct pg_texture* tex);
void pg_shader_text_write(struct pg_shader* shader, const char* str,
                          vec2 start, vec2 size, float space);

int pg_shader_2d(struct pg_shader* shader);
void pg_shader_2d_set_texture(struct pg_shader* shader, GLint tex_unit);

int pg_shader_3d(struct pg_shader* shader);
void pg_shader_3d_set_texture(struct pg_shader* shader, struct pg_texture* tex);

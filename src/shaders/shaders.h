/*  My shader implementations   */
int pg_shader_2d(struct pg_shader* shader);
void pg_shader_2d_set_texture(struct pg_shader* shader, GLint tex_unit);

int pg_shader_3d(struct pg_shader* shader);
void pg_shader_3d_set_texture(struct pg_shader* shader,
                              int color_slot, int normal_slot);
void pg_shader_3d_set_light(struct pg_shader* shader, vec3 ambient_light,
                               vec3 light_dir, vec3 light_color);
void pg_shader_3d_set_fog(struct pg_shader* shader, vec2 fog_plane,
                             vec3 fog_color);

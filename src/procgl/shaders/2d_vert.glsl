#version 330

#define view_matrix     pg_matrix_view
#define model_matrix    pg_matrix_model
#define tex_tx          pg_tex_rect_0
uniform mat4 view_matrix;
uniform mat4 model_matrix;
uniform vec4 tex_tx;
uniform vec2 light_pos;

#define v_tex_weight v_height
in vec3 v_position;
in vec4 v_color;
in vec2 v_tex_coord;
in float v_tex_weight;

out vec4 f_color;
out vec2 f_tex_coord;
out float f_tex_weight;
out vec2 f_pos;
out vec2 f_light_pos;

void main()
{
    gl_Position = view_matrix * model_matrix * vec4(v_position.xy, 0, 1);
    f_pos = gl_Position.xy;
    f_light_pos = (view_matrix * vec4(light_pos, 0, 1)).xy;
    f_tex_coord = v_tex_coord * tex_tx.zw + tex_tx.xy;
    f_color = v_color;
    f_tex_weight = v_tex_weight;
}


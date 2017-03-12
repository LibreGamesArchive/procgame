#version 330

uniform mat4 model_matrix;

in vec2 v_position;
in vec4 v_color;
in vec2 v_tex_coord;
in float v_tex_weight;

out vec4 f_color;
out vec2 f_tex_coord;
out float f_tex_weight;

void main()
{
    gl_Position = vec4(v_position, 0, 1) * model_matrix;
    f_color = v_color;
    f_tex_coord = v_tex_coord;
    f_tex_weight = v_tex_weight;
}


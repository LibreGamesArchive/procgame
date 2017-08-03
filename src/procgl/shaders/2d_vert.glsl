#version 330

uniform mat4 transform;
uniform vec2 tex_offset;
uniform vec2 tex_scale;

in vec3 v_position;
in vec4 v_color;
in vec2 v_tex_coord;
in float v_tex_weight;

out vec4 f_color;
out vec2 f_tex_coord;
out float f_tex_weight;

void main()
{
    gl_Position = transform * vec4(v_position.xy, 0, 1);
    f_color = v_color;
    f_tex_coord = v_tex_coord * tex_scale + tex_offset;
    f_tex_weight = v_tex_weight;
}


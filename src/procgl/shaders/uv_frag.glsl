#version 330

uniform sampler2D tex;
uniform sampler2D norm;

in vec3 f_pos;
in vec2 f_tex_coord;
in vec3 f_normal;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(1, 1, 1, 1);
}


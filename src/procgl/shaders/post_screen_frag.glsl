#version 330

uniform sampler2D color;
uniform vec2 resolution;

in vec2 f_tex_coord;

out vec4 frag_color;

void main()
{
    frag_color = texture(color, f_tex_coord);
}

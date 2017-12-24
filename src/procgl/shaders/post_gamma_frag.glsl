#version 330

uniform sampler2D color;
uniform vec2 resolution;
uniform float gamma;

in vec2 f_tex_coord;

out vec4 frag_color;

void main()
{
    vec4 in_color = texture(color, f_tex_coord);
    frag_color = vec4(pow(in_color.rgb, vec3(1.0/gamma)), in_color.a);
//    frag_color = vec4(gamma, gamma, gamma, gamma);
}

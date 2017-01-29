#version 330

uniform sampler2D tex;

in vec4 f_color;
in vec2 f_tex_coord;
in float f_tex_weight;

out vec4 frag_color;

void main()
{
    vec4 tex_color = texture(tex, f_tex_coord);
    frag_color = mix(tex_color, f_color, f_tex_weight);
    //frag_color = vec4(f_tex_weight, 0, 0, 1);
}


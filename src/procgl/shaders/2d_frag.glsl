#version 330

uniform sampler2D tex;
uniform float tex_weight;
uniform vec4 color_mod;

in vec4 f_color;
in vec2 f_tex_coord;
in float f_tex_weight;

out vec4 frag_color;

void main()
{
    vec4 tex_color = texture(tex, f_tex_coord);
    frag_color = mix(f_color, tex_color, f_tex_weight * tex_weight) * color_mod;
    //frag_color = vec4(f_tex_coord.xy, 0, 1);
}


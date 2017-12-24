#version 330

uniform sampler2D color;
uniform vec2 resolution;
uniform int blur_dir;
uniform vec2 blur_scale;

in vec2 f_tex_coord;

out vec4 frag_color;

vec4 blur5(vec2 dir)
{
    vec4 c = vec4(0.0);
    vec2 off1 = vec2(1.3846153846) * dir;
    vec2 off2 = vec2(3.2307692308) * dir;
    c += texture(color, f_tex_coord) * 0.2270270270;
    c += texture(color, f_tex_coord + (off1 / resolution)) * 0.3162162162;
    c += texture(color, f_tex_coord - (off1 / resolution)) * 0.3162162162;
    c += texture(color, f_tex_coord + (off2 / resolution)) * 0.0702702703;
    c += texture(color, f_tex_coord - (off2 / resolution)) * 0.0702702703;
    return c;
}

void main()
{
    vec2 dir = vec2(blur_dir, 1 - blur_dir) * blur_scale;
    frag_color = blur5(dir);
}


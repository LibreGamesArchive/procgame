#version 330

uniform sampler2D color;
uniform vec2 resolution;
uniform int blur_dir;
uniform vec2 blur_scale;

in vec2 f_tex_coord;

out vec4 frag_color;

vec4 blur3(vec2 dir)
{
    vec4 c = vec4(0.0);
    vec2 off1 = vec2(1.3333333333333333) * dir;
    c += texture(color, f_tex_coord) * 0.29411764705882354;
    c += texture(color, f_tex_coord + (off1 / resolution)) * 0.35294117647058826;
    c += texture(color, f_tex_coord - (off1 / resolution)) * 0.35294117647058826;
    return c;
}

void main()
{
    vec2 dir = vec2(blur_dir, 1 - blur_dir) * blur_scale;
    frag_color = blur3(dir);
}


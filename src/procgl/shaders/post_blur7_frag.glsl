#version 330

uniform sampler2D color;
uniform vec2 resolution;
uniform int blur_dir;
uniform vec2 blur_scale;

in vec2 f_tex_coord;

out vec4 frag_color;

vec4 blur7(vec2 dir)
{
    vec4 c = vec4(0.0);
    vec2 off1 = vec2(1.411764705882353) * dir;
    vec2 off2 = vec2(3.2941176470588234) * dir;
    vec2 off3 = vec2(5.176470588235294) * dir;
    c += texture(color, f_tex_coord) * 0.1964825501511404;
    c += texture(color, f_tex_coord + (off1 / resolution)) * 0.2969069646728344;
    c += texture(color, f_tex_coord - (off1 / resolution)) * 0.2969069646728344;
    c += texture(color, f_tex_coord + (off2 / resolution)) * 0.09447039785044732;
    c += texture(color, f_tex_coord - (off2 / resolution)) * 0.09447039785044732;
    c += texture(color, f_tex_coord + (off3 / resolution)) * 0.010381362401148057;
    c += texture(color, f_tex_coord - (off3 / resolution)) * 0.010381362401148057;
    return c;
}

void main()
{
    vec2 dir = vec2(blur_dir, 1 - blur_dir) * blur_scale;
    frag_color = blur7(dir);
}



#version 330

uniform sampler2D font;
in vec2 f_tex;
in vec4 f_color;
out vec4 frag_color;

void main()
{
    vec4 f = texture(font, f_tex) * f_color;
    frag_color = f;
}

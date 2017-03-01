#version 330

uniform sampler2D font;
in vec2 f_tex;
out vec4 frag_color;

void main()
{
    vec4 f = texture(font, f_tex);
    if(f.a < 0.5) discard;
    frag_color = f;
}

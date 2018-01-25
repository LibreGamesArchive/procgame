#version 330

uniform sampler2D pg_fbtexture_0;

in vec2 f_tex_coord;

out vec4 frag_color;

void main()
{
    frag_color = texture(pg_fbtexture_0, f_tex_coord);
}

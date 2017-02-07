#version 330

uniform sampler2D tex;
in vec2 f_tex_coord;

out vec4 frag_color;

void main()
{
    vec2 new_coord = vec2( f_tex_coord.x,
                           f_tex_coord.y + sin(f_tex_coord.x * 50) * 0.01 );

    frag_color = texture(tex, new_coord);
}

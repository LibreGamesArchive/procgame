#version 330

uniform sampler2D pg_fbtexture_0;
uniform float amplitude;
uniform float frequency;
uniform float phase;
uniform int axis;

in vec2 f_tex_coord;

out vec4 frag_color;

void main()
{
    vec2 new_coord = f_tex_coord;
    float sin_offset;
    if(axis == 0) {
        sin_offset = sin(new_coord.y * frequency + phase) * amplitude;
        new_coord.x += sin_offset;
    } else {
        sin_offset = sin(new_coord.x * frequency + phase) * amplitude;
        new_coord.y += sin_offset;
    }
    frag_color = texture(pg_fbtexture_0, new_coord);
}

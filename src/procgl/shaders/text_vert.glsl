#version 330

const vec2 verts[4] =
    vec2[](vec2(0, 0), vec2(1, 0), vec2(0, 1), vec2(1, 1));

/*  Number of glyphs per row in the texture */
uniform uint font_pitch;
/*  Size in UV coords of each glyph */
uniform vec2 glyph_size;
/*  Size in pixels of the screen    */
uniform vec2 screen_size;
/*  Position of the top left pixel of the first drawn character */
uniform vec2 start_pos;
/*  Size in pixels to draw each character   */
uniform vec2 char_size;
/*  Percentage of one character size to space each character    */
uniform float spacing;
/*  128-character string packed into uints  */
uniform uint string[32];

#define STR_CHAR(i) (((string[i / 4u] >> ((i % 4u) * 8u)) & 0xFFu) - 32u)

out vec2 f_tex;

void main()
{
    uint i = uint(gl_InstanceID);
    uint ch = STR_CHAR(i);
    vec2 vert = verts[gl_VertexID];
    vec2 pos = vec2(
        start_pos.x + gl_InstanceID * (char_size.x + spacing * char_size.x),
        screen_size.y - char_size.y - start_pos.y) + (vert * char_size);
    f_tex = vec2(ch % font_pitch * glyph_size.x + glyph_size.x * vert.x,
                 ch / font_pitch * glyph_size.y + glyph_size.y * (1 - vert.y));
    gl_Position = vec4((pos / screen_size) * 2 - 1, 0, 1);
}


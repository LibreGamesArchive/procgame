#version 330

const vec2 verts[6] =
    vec2[](vec2(0.00001, 0.00001), vec2(0.00001, 0.99999), vec2(0.99999, 0.00001),
           vec2(0.99999, 0.00001), vec2(0.00001, 0.99999), vec2(0.99999, 0.99999));

uniform mat4 transform;
/*  Number of glyphs per row in the texture */
uniform uint font_pitch;
/*  Size in UV coords of each glyph */
uniform vec2 glyph_size;
/*  32x 64-char individually transformed blocks; packed to uints    */
uniform uint chars[32 * 16];
uniform vec4 style[32];
uniform vec4 color[32];

#define UINT_CHAR(c, i) (((c) >> (i * 8)) & uint(0xFF))

out vec2 f_tex;
out vec4 f_color;

void main()
{
    int vert_idx = int(gl_VertexID % 6);
    int char_idx = int(gl_VertexID / 6);
    int line_idx = char_idx / 64;
    int uint_idx = char_idx / 4;
    int char_idx_in_uint = char_idx % 4;
    int char_idx_in_line = char_idx % 64;
    uint ch = UINT_CHAR(chars[uint_idx], char_idx_in_uint);
    uint frame = ch - uint(32);
    vec4 tx = style[line_idx];
    vec2 vert = verts[vert_idx];
    vec2 tex_offset = vec2(frame % font_pitch * glyph_size.x,
                           frame / font_pitch * glyph_size.y);
    f_tex = vert * glyph_size + tex_offset;
    f_color = color[line_idx];
    gl_Position = transform *
        vec4(vert * tx.z + tx.xy + vec2(tx.z * tx.w * char_idx_in_line, 0), 0, 1);
}


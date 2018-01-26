#version 330

const vec2 verts[6] =
    vec2[](vec2(0.00001, 0.00001), vec2(0.00001, 0.99999), vec2(0.99999, 0.00001),
           vec2(0.99999, 0.00001), vec2(0.00001, 0.99999), vec2(0.99999, 0.99999));

uniform mat4 pg_matrix_mvp;
/*  Number of glyphs per row in the texture */
uniform uint font_pitch;
/*  Size of each glyph in UV (x,y) and actual units (z,w)   */
uniform vec4 glyph_size;
/*  64-char string (packed to uints)    */
uniform uint chars[16];
/*  Pos (x, y), Glyph size (z), Spacing (w) */
uniform vec4 style; 
uniform vec4 color;
uniform vec2 dir;

#define UINT_CHAR(c, i) (((c) >> (i * 8)) & uint(0xFF))

out vec2 f_tex;
out vec4 f_color;

void main()
{
    int vert_idx = int(gl_VertexID % 6);
    int char_idx = int(gl_VertexID / 6);
    int uint_idx = char_idx / 4;
    int char_idx_in_uint = char_idx % 4;
    uint ch = UINT_CHAR(chars[uint_idx], char_idx_in_uint);
    uint frame = ch - uint(32);
    vec2 tex_offset = vec2(frame % font_pitch * glyph_size.x,
                           frame / font_pitch * glyph_size.y);
    float glyph_aspect = glyph_size.z / glyph_size.w;
    vec2 glyph_offset =
        vec2(style.z * style.w * glyph_aspect * char_idx * dir.x,
             style.z * style.w * char_idx * dir.y);
    vec2 vert = verts[vert_idx];
    f_tex = vert * glyph_size.xy + tex_offset;
    vert.x *= glyph_aspect;
    vert = vert * style.z + style.xy + glyph_offset;
    f_color = color;
    gl_Position = pg_matrix_mvp * vec4(vert.xy, 0, 1);
}


#version 330

const vec2 verts[4] =
    vec2[](vec2(-1, -1), vec2(1, -1), vec2(-1, 1), vec2(1, 1));

out vec2 f_tex_coord;

void main()
{
    gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
    f_tex_coord = (verts[gl_VertexID] + 1) / 2;
}

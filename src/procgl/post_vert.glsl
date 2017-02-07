#version 330

in vec2 v_position;
out vec2 f_tex_coord;

void main()
{
    gl_Position = vec4(v_position, 0.0, 1.0);
    f_tex_coord = (v_position + 1) / 2;
}

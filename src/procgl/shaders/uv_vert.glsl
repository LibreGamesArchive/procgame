#version 330

in vec3 v_position;
in vec3 v_normal;
in vec2 v_tex_coord;

out vec3 f_normal;
out vec2 f_tex_coord;
out vec3 f_pos;

void main()
{
    gl_Position = vec4(v_tex_coord * 2 - 1, 0, 1);
    f_pos = v_position;
    f_normal = v_normal;
    f_tex_coord = v_tex_coord;
}


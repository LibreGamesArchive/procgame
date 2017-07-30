#version 330

uniform mat4 model_matrix;
uniform mat4 normal_matrix;
uniform mat4 view_matrix;
uniform mat4 projview_matrix;
uniform vec2 tex_scale;
uniform vec2 tex_offset;

in vec3 v_position;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_tex_coord;

out vec3 f_normal;
out vec3 f_tangent;
out vec3 f_bitangent;
out vec2 f_tex_coord;

void main()
{
    mat3 modelview = mat3(view_matrix) * mat3(model_matrix);
    gl_Position = projview_matrix * model_matrix * vec4(v_position, 1.0);
    f_normal = mat3(normal_matrix) * v_normal;
    f_tangent = mat3(normal_matrix) * v_tangent;
    f_bitangent = mat3(normal_matrix) * v_bitangent;
    f_tex_coord = v_tex_coord * tex_scale + tex_offset;
}


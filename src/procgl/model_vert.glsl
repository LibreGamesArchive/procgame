#version 330

uniform mat4 model_matrix;

layout(std140) uniform vert_base {
    mat4 proj_matrix;
    mat4 view_matrix;
    mat4 projview_matrix;
};

in vec3 v_position;
in vec3 v_normal;
in vec3 v_tangent;
in vec3 v_bitangent;
in vec2 v_tex_coord;

out float f_depth;
out vec3 f_normal;
out vec3 f_tangent;
out vec3 f_bitangent;
out vec2 f_tex_coord;

void main()
{
    gl_Position = projview_matrix * model_matrix * vec4(v_position, 1.0);
    f_depth = length(view_matrix * model_matrix * vec4(v_position, 1.0));
    f_normal = mat3(model_matrix) * v_normal;
    f_tangent = mat3(model_matrix) * v_tangent;
    f_bitangent = mat3(model_matrix) * v_bitangent;
    f_tex_coord = v_tex_coord;
}


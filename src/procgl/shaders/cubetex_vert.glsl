#version 330

uniform mat4 model_matrix;
uniform mat4 normal_matrix;
uniform mat4 proj_matrix;
uniform mat4 view_matrix;
uniform mat4 projview_matrix;

uniform float blend_sharpness;

in vec3 v_position;
in vec3 v_normal;
in vec4 v_color;

out float f_depth;
out vec3 f_mpos;
out vec3 f_blend;
out vec3 f_pos;
out vec3 f_normal;
out vec3 f_v_normal;
out vec3 f_tangent;
out vec3 f_bitangent;
out vec4 f_color;

void main()
{
    gl_Position = projview_matrix * model_matrix * vec4(v_position, 1.0);
    f_mpos = v_position * 4;
    f_pos = vec3(model_matrix * vec4(v_position, 1.0));
    f_depth = length(view_matrix * model_matrix * vec4(v_position, 1.0));
    f_normal = mat3(normal_matrix) * v_normal;
    f_v_normal = v_normal;
    f_tangent = vec3(0, 0, 1) * v_normal.x +
                vec3(1, 0, 0) * v_normal.y +
                vec3(1, 0, 0) * v_normal.z;
    f_bitangent = vec3(0, 1, 0) * v_normal.x +
                  vec3(0, 0, 1) * v_normal.y +
                  vec3(0, 1, 0) * v_normal.z;
    f_blend = pow(abs(v_normal).yzx, vec3(blend_sharpness, blend_sharpness, blend_sharpness));
    f_blend /= (f_blend.x + f_blend.y + f_blend.z);
    f_color = v_color;
}


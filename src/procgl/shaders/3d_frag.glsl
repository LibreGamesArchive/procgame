#version 330

uniform sampler2D tex;
uniform sampler2D norm;

in vec2 f_tex_coord;
in vec3 f_normal;
in vec3 f_tangent;
in vec3 f_bitangent;

layout(location = 0) out vec4 g_albedo;
layout(location = 1) out vec4 g_normal;

void main()
{
    mat3 tbn = mat3(f_tangent.x, f_bitangent.x, f_normal.x,
                    f_tangent.y, f_bitangent.y, f_normal.y,
                    f_tangent.z, f_bitangent.z, f_normal.z);
    vec4 tex_color = texture(tex, f_tex_coord);
    if(tex_color.a < 0.5) discard;
    vec4 light_tex = texture(norm, f_tex_coord);
    vec4 tex_norm = vec4(light_tex.xyz * 2 - 1, light_tex.w);
    g_albedo = tex_color;
    g_normal = vec4(tex_norm.xyz * tbn * 0.5 + 0.5, tex_norm.w);
}

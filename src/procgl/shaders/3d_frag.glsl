#version 330

uniform sampler2D tex;
uniform sampler2D norm;

/*  Distance from the viewer    */
in float f_height;
in float f_depth;
in vec3 f_pos;
/*  Texture coord for all four textures */
in vec2 f_tex_coord;
/*  Vertex normals  */
in vec3 f_normal;
/*  Calculated vertex tangent and bitangent  */
in vec3 f_tangent;
in vec3 f_bitangent;

layout(location = 0) out vec4 g_albedo;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec4 g_pos;

void main()
{
    mat3 tbn = mat3(f_tangent.x, f_bitangent.x, f_normal.x,
                    f_tangent.y, f_bitangent.y, f_normal.y,
                    f_tangent.z, f_bitangent.z, f_normal.z);
    vec4 tex_color = texture(tex, f_tex_coord);
    if(tex_color.a < 0.5) discard;
    vec4 tex_norm = vec4(texture(norm, f_tex_coord).rgb * 2 - 1, 1);
    g_albedo = tex_color;
    g_normal = vec4(tex_norm.xyz * tbn * 0.5 + 0.5, 1);
    g_pos = vec4(f_pos.xyz, f_depth);
}
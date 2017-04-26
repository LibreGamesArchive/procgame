#version 330

uniform sampler2D tex_front;
uniform sampler2D tex_back;
uniform sampler2D tex_left;
uniform sampler2D tex_right;
uniform sampler2D tex_top;
uniform sampler2D tex_bottom;

uniform sampler2D norm_front;
uniform sampler2D norm_back;
uniform sampler2D norm_left;
uniform sampler2D norm_right;
uniform sampler2D norm_top;
uniform sampler2D norm_bottom;

uniform vec2 tex_scale[6];

/*  Distance from the viewer    */
in float f_depth;
in vec3 f_mpos;
in vec3 f_pos;
in vec3 f_normal;
in vec3 f_v_normal;
in vec3 f_tangent;
in vec3 f_bitangent;
in vec3 f_blend;

layout(location = 0) out vec4 g_albedo;
layout(location = 1) out vec4 g_normal;
layout(location = 2) out vec4 g_pos;

void main()
{
    mat3 tbn = mat3(f_tangent.x, f_bitangent.x, f_normal.x,
                    f_tangent.y, f_bitangent.y, f_normal.y,
                    f_tangent.z, f_bitangent.z, f_normal.z);
    vec4 tex_c0, tex_c1, tex_c2;
    vec4 norm_c0, norm_c1, norm_c2;
    /*  Get the diffuse texture samples */
    if(f_v_normal.y < 0) tex_c0 = texture(tex_front, f_mpos.xz * tex_scale[0]);
    else tex_c0 = texture(tex_back, f_mpos.xz * tex_scale[1]);
    if(f_v_normal.z < 0) tex_c1 = texture(tex_bottom, f_mpos.xy * tex_scale[2]);
    else tex_c1 = texture(tex_top, f_mpos.xy * tex_scale[3]);
    if(f_v_normal.x < 0) tex_c2 = texture(tex_left, f_mpos.yz * tex_scale[4]);
    else tex_c2 = texture(tex_right, f_mpos.yz * tex_scale[5]);
    /*  Get the normal texture samples  */
    if(f_v_normal.y < 0) norm_c0 = texture(norm_front, f_mpos.xz * tex_scale[0]);
    else norm_c0 = texture(norm_back, f_mpos.xz * tex_scale[1]);
    if(f_v_normal.z < 0) norm_c1 = texture(norm_top, f_mpos.xy * tex_scale[2]);
    else norm_c1 = texture(norm_bottom, f_mpos.xy * tex_scale[3]);
    if(f_v_normal.x < 0) norm_c2 = texture(norm_left, f_mpos.yz * tex_scale[4]);
    else norm_c2 = texture(norm_right, f_mpos.yz * tex_scale[5]);
    norm_c2.xy = 1 - norm_c2.xy;
    /*  Blend them based on the vertex normals  */
    vec4 tex_blend = tex_c0 * f_blend.x +
                     tex_c1 * f_blend.y +
                     tex_c2 * f_blend.z;
    vec4 norm_blend = norm_c0 * f_blend.x +
                      norm_c1 * f_blend.y +
                      norm_c2 * f_blend.z;
    norm_blend = vec4(norm_blend.xyz * 2 - 1, norm_blend.w);
    g_albedo = tex_blend;
    g_normal = vec4(norm_blend.xyz * tbn * 0.5 + 0.5, norm_blend.w);
    g_pos = vec4(f_pos.xyz, f_depth);
}

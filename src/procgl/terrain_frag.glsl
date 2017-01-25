#version 330

/*  Terrain renderer (Single biome)
    Uses four textures, split into two groups of base and detail textures,
    which are then blended based on the resulting height at each sample */

layout(std140) uniform frag_base {
    vec3 amb_color;
    vec3 sun_dir;
    vec3 sun_color;
    vec2 fog_plane;
    vec3 fog_color;
};

layout(std140) uniform terrain_data {
    vec4 tex_height_mod;
    vec4 tex_scale;
    vec2 tex_detail_weight;
};

uniform sampler2D diffuse_map[4];
uniform sampler2D normal_map[4];
uniform sampler2D extra_map[4];

/*  Distance from the viewer    */
in float f_height;
in float f_depth;
/*  Texture coord for all four textures */
in vec2 f_tex_coord;
/*  Vertex normals  */
in vec3 f_normal;
/*  Calculated vertex tangent and bitangent  */
in vec3 f_tangent;
in vec3 f_bitangent;

out vec4 frag_color;

void main()
{
    /*  First we make these three vec3's into a mat3 for the TBN matrix,
        and get the light direction in tangent space    */
    mat3 tbn = mat3(f_tangent, f_bitangent, f_normal);
    vec3 sun_dir_tan = sun_dir * tbn;
    sun_dir_tan = normalize(sun_dir_tan);
    /*  Now we sample every texture from every slot;
        Combining the base and detail texture for the upper and lower terrain
        textures.   */
    vec4 tex_diff[2], tex_norm[2];
    tex_diff[0] = mix(texture(diffuse_map[0], f_tex_coord * tex_scale.x),
                      texture(diffuse_map[1], f_tex_coord * tex_scale.y), tex_detail_weight.x);
    tex_diff[1] = mix(texture(diffuse_map[2], f_tex_coord * tex_scale.z),
                      texture(diffuse_map[3], f_tex_coord * tex_scale.w), tex_detail_weight.y);
    tex_norm[0] = mix(texture(normal_map[0], f_tex_coord * tex_scale.x),
                      texture(normal_map[1], f_tex_coord * tex_scale.y), tex_detail_weight.x);
    tex_norm[1] = mix(texture(normal_map[2], f_tex_coord * tex_scale.z),
                      texture(normal_map[3], f_tex_coord * tex_scale.w), tex_detail_weight.y);
    /*  Apply the height mod    */
    tex_norm[0].w += f_height * 0.75 + tex_height_mod.x + tex_height_mod.y * tex_detail_weight.x;
    tex_norm[1].w += f_height * 1 + tex_height_mod.z + tex_height_mod.w * tex_detail_weight.y;
    /*  The magic is here: take the difference between the final two heights
        as a value in [0,1], representing the height of the second texture pair
        over the first texture pair (plus 0.5). This is then shifted so that
        values above or below a certain height difference result in NO diffuse
        texture blending; one takes over entirely   */
    float height_diff = (tex_norm[1].w - tex_norm[0].w) * 0.5 + 0.5;
    height_diff = smoothstep(0.45, 0.55, height_diff);
    vec4 tex_color = mix(tex_diff[1], tex_diff[0], height_diff);
    vec3 bump_norm = mix(tex_norm[1], tex_norm[0], height_diff).xyz * 2 - 1;
    float reflectance = max(dot(sun_dir_tan, bump_norm.rgb), 0.0);
    vec4 draw_color = (reflectance * tex_color * vec4(sun_color, 1));
    draw_color.rgb += (amb_color * tex_color.rgb);
    float fog_depth = clamp((fog_plane[1] - f_depth) / (fog_plane[1] - fog_plane[0]), 0.0, 1.0);
    frag_color = mix(vec4(fog_color, 1.0), draw_color, fog_depth);
}

#version 330

layout(std140) uniform frag_base {
    vec3 amb_color;
    vec3 sun_dir;
    vec3 sun_color;
    vec2 fog_plane;
    vec3 fog_color;
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
    mat3 tbn = mat3(f_tangent, f_bitangent, f_normal);
    vec3 sun_dir_tan = sun_dir * tbn;
    sun_dir_tan = normalize(sun_dir_tan);
    vec4 tex_color = texture(diffuse_map[0], f_tex_coord);
    vec3 tex_norm = texture(normal_map[0], f_tex_coord).rgb * 2 - 1;
    float reflectance = max(dot(sun_dir_tan, tex_norm), 0.0);
    vec4 draw_color = (vec4(amb_color, 1) * tex_color) +
                      (tex_color * vec4(sun_color, 1) * reflectance);
    float fog_depth = clamp((fog_plane[1] - f_depth) / (fog_plane[1] - fog_plane[0]), 0.0, 1.0);
    frag_color = mix(vec4(fog_color, 1.0), draw_color, fog_depth);
    //frag_color = vec4(sun_dir_tan, 1);
}

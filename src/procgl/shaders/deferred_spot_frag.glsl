#version 330

uniform mat4 view_matrix;
uniform sampler2D g_normal;
uniform sampler2D g_depth;
uniform vec3 eye_pos;
uniform vec2 clip_planes;

flat in vec4 f_light;
flat in vec4 f_dir_angle;
flat in vec3 f_color;

in vec4 pos_cs;
in vec3 view_ray;
out vec4 frag_color;

void main()
{
    /*  Reconstructing world-space position from the depth buffer   */
    vec3 look_dir =  vec3(view_matrix[0][2],
                          view_matrix[1][2],
                          view_matrix[2][2]);
    vec2 g_tex_coord = (pos_cs.xy / pos_cs.w) * 0.5 + 0.5;
    vec3 view_ray = normalize(view_ray);
    float depth = texture(g_depth, g_tex_coord).x;
    float lin_depth = clip_planes[1] / (clip_planes[0] - depth);
    float viewzdist = dot(look_dir, view_ray);
    vec3 frag_to_eye = view_ray * (lin_depth / viewzdist);
    vec3 pos = eye_pos + frag_to_eye;
    frag_to_eye = -normalize(frag_to_eye);
    /*  Calculate distance from the light to the fragment   */
    vec3 light_to_pos = pos - f_light.xyz;
    float dist = length(light_to_pos);
    if(dist > f_light.w) discard;
    /*  Light the fragment based on the normal  */
    vec4 norm_sample = texture(g_normal, g_tex_coord);
    vec3 norm = norm_sample.xyz * 2 - 1;
    vec3 light_dir = normalize(light_to_pos);
    float light_angle = acos(dot(light_dir, f_dir_angle.xyz));
    float shininess = norm_sample.w;
    vec3 half_angle = normalize(frag_to_eye - light_dir);
    vec3 specular = f_color * shininess * pow(max(dot(norm, half_angle), 0), 16);
    vec3 diffuse = f_color * max(dot(norm, -light_dir), 0);
    float attenuation = pow(1 - dist / f_light.w, 1.7);
    frag_color = vec4(attenuation * (diffuse + specular)
            * clamp((f_dir_angle.w - light_angle) * 20, 0, 1), 0);
}

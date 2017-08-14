#version 330

uniform mat4 view_matrix;
uniform sampler2D g_normal;
uniform sampler2D g_depth;
uniform vec4 light;
uniform vec4 dir_angle;
uniform vec3 color;
uniform vec3 eye_pos;
uniform vec2 clip_planes;

in vec4 pos_cs;
in vec3 view_ray;
out vec4 frag_color;

void main()
{
    vec3 look_dir =  vec3(view_matrix[0][2],
                          view_matrix[1][2],
                          view_matrix[2][2]);
    vec2 g_tex_coord = (pos_cs.xy / pos_cs.w) * 0.5 + 0.5;
    vec3 view_ray = normalize(view_ray);
    float depth = texture(g_depth, g_tex_coord).x;
    float lin_depth = clip_planes[1] / (clip_planes[0] - depth);
    float viewzdist = dot(look_dir, view_ray);
    vec3 pos = eye_pos + view_ray * (lin_depth / viewzdist);
    vec3 light_to_pos = pos - light.xyz;
    float dist = length(light_to_pos);
    if(dist > light.w) discard;
    vec4 norm_sample = texture(g_normal, g_tex_coord);
    vec3 norm = norm_sample.xyz * 2 - 1;
    float shininess = norm_sample.w;
    vec3 light_dir = -normalize(light_to_pos);
    vec3 dir_norm = normalize(dir_angle.xyz);
    float light_angle = acos(dot(-light_dir, dir_norm));
    if(light_angle > dir_angle.w) discard;
    vec3 frag_to_eye = normalize(eye_pos - pos);
    vec3 half_angle = normalize(frag_to_eye + light_dir);
    vec3 specular = color * shininess * pow(max(dot(norm, half_angle), 0), 32);
    vec3 diffuse = color * max(dot(norm, light_dir), 0);
    float attenuation = pow(1 - dist / light.w, 1.7);
    frag_color = vec4(attenuation * (diffuse + specular), 0);
    //frag_color = vec4(-dir_angle.xyz, 1);
}

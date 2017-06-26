#version 330

uniform sampler2D g_normal;
uniform sampler2D g_pos;
uniform vec4 light;
uniform vec3 color;
uniform vec3 eye_pos;

in vec4 f_pos;
out vec4 frag_color;

void main()
{
    vec2 g_tex_coord = (f_pos.xy / f_pos.w) * 0.5 + 0.5;
    vec3 pos = texture(g_pos, g_tex_coord).xyz;
    vec4 norm_sample = texture(g_normal, g_tex_coord);
    vec3 norm = norm_sample.xyz * 2 - 1;
    float shininess = norm_sample.w;
    vec3 light_to_pos = light.xyz - pos;
    float dist = length(light_to_pos);
    if(dist > light.w) discard;
    vec3 light_dir = normalize(light_to_pos);
    vec3 frag_to_eye = normalize(eye_pos - pos);
    vec3 half_angle = normalize(frag_to_eye + light_dir);
    vec3 specular = color * shininess * pow(max(dot(norm, half_angle), 0), 32);
    vec3 diffuse = color * max(dot(norm, light_dir), 0);
    float attenuation = 1 - dist / light.w;
    frag_color = vec4(attenuation * (diffuse + specular), 0);
}

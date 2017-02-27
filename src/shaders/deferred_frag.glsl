#version 330

uniform sampler2D g_normal;
uniform sampler2D g_pos;
uniform vec4 light;
uniform vec3 color;

in vec4 pos;
out vec4 frag_color;

void main()
{
    vec2 g_tex_coord = (pos.xy / pos.w) * 0.5 + 0.5;
    vec3 pos = texture(g_pos, g_tex_coord).xyz;
    vec3 normal = texture(g_normal, g_tex_coord).xyz * 2 - 1;
    vec3 light_to_pos = pos - light.xyz;
    float dist = length(light_to_pos);
    if(dist > light.w) discard;
    vec3 light_dir = normalize(-light_to_pos);
    float attenuation = 1 - dist / light.w;
    frag_color = vec4(color * attenuation * max(0, dot(normal, light_dir)), 0);
}

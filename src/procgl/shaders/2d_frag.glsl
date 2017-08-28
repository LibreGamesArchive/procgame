#version 330

uniform sampler2D tex;
uniform sampler2D norm;
uniform float tex_weight;
uniform vec4 color_mod;
uniform vec3 light_color;
uniform vec3 ambient_color;

in vec4 f_color;
in vec2 f_tex_coord;
in float f_tex_weight;
in vec2 f_pos;
in vec2 f_light_pos;

out vec4 frag_color;

void main()
{
    vec4 tex_color = texture(tex, f_tex_coord);
    tex_color = mix(f_color, tex_color, f_tex_weight * tex_weight) * color_mod;
    vec4 tex_norm = texture(norm, f_tex_coord);
    vec3 norm = normalize(tex_norm.xyz * 2 - 1);
    vec3 light_to_pos = vec3(f_light_pos - f_pos, 0);
    vec3 light_dir = normalize(light_to_pos);
    float light = max(0, dot(-light_dir, norm));
    if(tex_norm.w == 1) frag_color = tex_color;
    else {
        frag_color = vec4(tex_color.rgb *
            (light_color * light + ambient_color), tex_color.a);
    }
}


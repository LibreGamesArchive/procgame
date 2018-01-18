#version 330

uniform float tex_weight;
uniform vec4 color_mod;
uniform vec4 color_add;
uniform vec3 light_color;
uniform vec3 ambient_color;

#define normal_matrix   pg_matrix_normal
#define tex             pg_texture_0
#define norm            pg_texture_1
uniform mat4 normal_matrix;
uniform sampler2D tex;
uniform sampler2D norm;

in vec4 f_color;
in vec2 f_tex_coord;
in float f_tex_weight;
in vec2 f_pos;
in vec2 f_light_pos;

out vec4 frag_color;

void main()
{
    vec4 tex_color = texture(tex, f_tex_coord);
    tex_color = mix(f_color, tex_color, f_tex_weight * tex_weight) * color_mod + color_add;
    vec4 tex_norm = texture(norm, f_tex_coord);
    vec3 norm = mat3(normal_matrix) * normalize(tex_norm.xyz * 2 - 1);
    norm.y = -norm.y;
    vec3 light_to_pos = vec3(f_pos - f_light_pos, -0.2);
    vec3 light_dir = normalize(light_to_pos);
    float light = max(0, dot(-light_dir, norm));
    float spec = max(0, pow(dot(-light_dir, norm), 32)) * tex_norm.a;
    light += spec;
    vec4 final;
    final = tex_color;
    if(tex_norm.w == 1) final = tex_color;
    else {
        final = vec4(tex_color.rgb *
            (light_color * light + ambient_color), tex_color.a);
    }
    frag_color = clamp(final, 0, 1);
    //frag_color[0] = tex_weight;
    //frag_color[1] = f_tex_weight;
    //frag_color = vec4(1,1,1,1);
    //frag_color = vec4(f_tex_coord.x, f_tex_coord.y, 0, 1);
    //frag_color = vec4(1, 0, 0, 1);
}


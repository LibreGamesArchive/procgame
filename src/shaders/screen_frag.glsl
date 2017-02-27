#version 330

uniform sampler2D color;
uniform sampler2D light;
uniform vec3 ambient_light;
in vec2 f_tex_coord;
out vec4 frag_color;

void main()
{
    frag_color = vec4(texture(color, f_tex_coord).rgb *
        (texture(light, f_tex_coord).rgb + ambient_light), 1);
}


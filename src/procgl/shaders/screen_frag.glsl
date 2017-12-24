#version 330

uniform sampler2D color;
uniform sampler2D light;
uniform sampler2D norm;
uniform vec3 ambient_light;
in vec2 f_tex_coord;
out vec4 frag_color;

void main()
{
    frag_color = vec4(0, 0, 0, 0);
    if(texture(norm, f_tex_coord).w == 1.0)
        frag_color = texture(color, f_tex_coord);
    frag_color += vec4(texture(color, f_tex_coord).rgb *
        (texture(light, f_tex_coord).rgb + ambient_light), 1);
    //frag_color = texture(norm, f_tex_coord);
}


#version 330

uniform sampler2D font;
uniform vec3 normal_3d;
uniform int is_3d;
in vec2 f_tex;
in vec4 f_color;

layout(location = 0) out vec4 g_albedo;
layout(location = 1) out vec4 g_normal;

void main()
{
    vec4 f = texture(font, f_tex);
    g_albedo = f * f_color;
    if(is_3d == 1) {
        if(f.a < 0.5) discard;
        g_normal = vec4(normal_3d.xyz * 0.5 + 0.5, f_color.a);
    }
}

#version 330

#define BLUR_RADIUS 20

uniform sampler2D pg_fbtexture_0;
uniform vec2 resolution;
uniform vec2 blur_dir;

in vec2 f_tex_coord;

out vec4 frag_color;

float sigmoid(float x)
{
    x = x * 2.0 - 1.0;
    return -x * abs(x) * 0.5 + x + 0.5;
}

vec4 blur(vec2 uv)
{
    vec4 tex_color = vec4(0);
    vec4 color = vec4(0);
    vec2 size = blur_dir / resolution;
    float divisor = 0.0;
    float weight = 0.0;
    float radius_mult = 1.0 / BLUR_RADIUS;
    for (float t = -BLUR_RADIUS; t <= BLUR_RADIUS; t++)
    {
        tex_color = texture(pg_fbtexture_0, uv + (t * size));
        weight = sigmoid(1.0 - (abs(t) * radius_mult)); 
        color += tex_color * weight; 
        divisor += weight; 
    }
    return color / divisor;
}

void main()
{
    frag_color = blur(f_tex_coord.xy); 
}

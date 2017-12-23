#version 330

#define VOLUME_SIZE 1.1

const vec3 light_verts[36] = vec3[](
    vec3(-VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE, -VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3(-VOLUME_SIZE,  VOLUME_SIZE,  VOLUME_SIZE),
    vec3( VOLUME_SIZE, -VOLUME_SIZE,  VOLUME_SIZE));

uniform mat4 projview_matrix;
uniform vec3 eye_pos;
uniform vec4 light[64];
uniform vec3 color[64];
out vec4 pos_cs;
out vec3 view_ray;
flat out vec4 f_light;
flat out vec3 f_color;

void main()
{
    f_light = light[gl_VertexID / 36];
    f_color = color[gl_VertexID / 36];
    vec3 pos_ws = (light_verts[gl_VertexID % 36]) * f_light.w + f_light.xyz;
    gl_Position = projview_matrix * vec4(pos_ws, 1.0);
    pos_cs = gl_Position;
    view_ray = eye_pos - pos_ws;
}


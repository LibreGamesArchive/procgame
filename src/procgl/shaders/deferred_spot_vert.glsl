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
uniform vec4 dir_quat[64];
uniform vec4 cone_and_color[64];
out vec4 pos_cs;
out vec3 view_ray;
flat out vec4 f_light;
flat out vec4 f_dir_angle;
flat out vec3 f_color;

vec3 quat_mul(vec4 q, vec3 v)
{
    vec3 t = 2 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}

void main()
{
    int l = gl_VertexID / 36;
    f_light = light[l];
    f_dir_angle = vec4(quat_mul(dir_quat[l], vec3(0, 0, -1)), cone_and_color[l].w);
    f_color = cone_and_color[l].xyz;
    float angle_scale = sin(cone_and_color[l].w);
    vec3 lv = light_verts[gl_VertexID] * vec3(angle_scale, angle_scale, 0.5) + vec3(0, 0, -0.5);
    vec3 pos_ws = quat_mul(dir_quat[l], lv) * f_light.w + f_light.xyz;
    gl_Position = projview_matrix * vec4(pos_ws, 1.0);
    pos_cs = gl_Position;
    view_ray = eye_pos - pos_ws;
}


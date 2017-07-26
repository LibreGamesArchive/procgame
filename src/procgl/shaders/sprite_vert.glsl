#version 330

#define PG_SPRITE_SPHERICAL     0
#define PG_SPRITE_CYLINDRICAL   1

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;
uniform mat4 modelview_matrix;
uniform vec2 tex_scale;
uniform vec2 tex_offset;
uniform int mode;

in vec3 v_position;
in vec2 v_tex_coord;

out float f_depth;
out vec3 f_normal;
out vec3 f_tangent;
out vec3 f_bitangent;
out vec2 f_tex_coord;
out vec3 f_pos;

void main()
{
    mat4 sprite_modelview = modelview_matrix;
    mat4 sprite_model = model_matrix;
    f_normal =  vec3(view_matrix[0][2],
                     view_matrix[1][2],
                     view_matrix[2][2]);
    f_tangent =  vec3(view_matrix[0][0],
                      view_matrix[1][0],
                      view_matrix[2][0]);
    if(mode == PG_SPRITE_SPHERICAL) {
        sprite_modelview[0].xyz = vec3(1, 0, 0);
        sprite_modelview[2].xyz = vec3(0, 1, 0);
        sprite_modelview[1].xyz = vec3(0, 0, 1);
        sprite_model[0].xyz = vec3(1, 0, 0);
        sprite_model[2].xyz = vec3(0, 1, 0);
        sprite_model[1].xyz = vec3(0, 0, 1);
        f_bitangent =  -vec3(view_matrix[0][1],
                             view_matrix[1][1],
                             view_matrix[2][1]);
    } else {
        sprite_modelview[0].xyz = vec3(1, 0, 0);
        sprite_modelview[1].xyz = vec3(0, 0, 1);
        sprite_model[0].xyz = vec3(1, 0, 0);
        sprite_model[1].xyz = vec3(0, 0, 1);
        f_normal = normalize(vec3(f_normal.xy, 0));
        f_tangent = normalize(vec3(f_tangent.xy, 0));
        f_bitangent = vec3(0, 0, -1);
    }
    gl_Position = proj_matrix * sprite_modelview * vec4(v_position, 1.0);
    f_pos = vec3(sprite_model * vec4(v_position, 1.0));
    f_depth = length(sprite_modelview * vec4(v_position, 1.0));
    f_tex_coord = v_tex_coord * tex_scale + tex_offset;
}



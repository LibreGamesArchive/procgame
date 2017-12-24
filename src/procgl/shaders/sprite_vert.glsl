#version 330

#define PG_SPRITE_SPHERICAL     0
#define PG_SPRITE_CYLINDRICAL   1

uniform mat4 view_matrix;
uniform mat4 proj_matrix;
uniform mat4 modelview_matrix;
/*  Texture transformation, and sprite transformation (on screen)   */
uniform vec4 sp_tx;
uniform vec4 tex_tx;
uniform int mode;

in vec3 v_position;
in vec2 v_tex_coord;

out vec2 f_tex_coord;
out vec3 f_normal;
out vec3 f_tangent;
out vec3 f_bitangent;

void main()
{
    mat4 sprite_modelview = modelview_matrix;
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
        f_bitangent =  -vec3(view_matrix[0][1],
                             view_matrix[1][1],
                             view_matrix[2][1]);
    } else {
        sprite_modelview[0].xyz = vec3(1, 0, 0);
        sprite_modelview[1].xyz = vec3(0, 0, 1);
        f_normal = normalize(vec3(f_normal.xy, 0));
        f_tangent = normalize(vec3(f_tangent.xy, 0));
        f_bitangent = vec3(0, 0, -1);
    }
    gl_Position = proj_matrix * sprite_modelview *
        vec4(v_position.x * sp_tx.x + sp_tx.z, 0.0,
             v_position.z * sp_tx.y + sp_tx.w, 1.0);
    f_tex_coord = v_tex_coord * tex_tx.xy + tex_tx.zw;
}



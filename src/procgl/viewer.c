#include "../linmath.h"
#include "viewer.h"


void pg_viewer_init(struct pg_viewer* view, vec3 pos, vec2 dir,
                    vec2 size, vec2 near_far)
{
    pg_viewer_set(view, pos, dir);
    vec2_dup(view->size, size);
    mat4_perspective(view->proj_matrix, 1.0f, size[0] / size[1],
                     near_far[0], near_far[1]);
}

void pg_viewer_set(struct pg_viewer* view, vec3 pos, vec2 dir)
{
    vec3_dup(view->pos, pos);
    vec2_dup(view->dir, dir);
    mat4_look_at(view->view_matrix,
                 (vec3){ 0.0f, 0.0f, 0.0f },
                 (vec3){ 0.0f, 1.0f, 0.0f },
                 (vec3){ 0.0f, 0.0f, 1.0f });
    mat4_rotate(view->view_matrix, view->view_matrix, 1, 0, 0, view->dir[1]);
    mat4_rotate(view->view_matrix, view->view_matrix, 0, 0, 1, view->dir[0]);
    mat4_translate_in_place(view->view_matrix,
                            -view->pos[0], -view->pos[1], -view->pos[2]);
}

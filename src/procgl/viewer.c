#include "ext/linmath.h"
#include "viewer.h"


void pg_viewer_init(struct pg_viewer* view, vec3 pos, vec2 dir,
                    vec2 size, vec2 near_far)
{
    pg_viewer_set(view, pos, dir);
    vec2_dup(view->size, size);
    vec2_dup(view->near_far, near_far);
    mat4_perspective(view->proj_matrix, 1.0f, size[0] / size[1],
                     near_far[0], near_far[1]);
}

void pg_viewer_set(struct pg_viewer* view, vec3 pos, vec2 dir)
{
    vec3_dup(view->pos, pos);
    vec2_dup(view->dir, dir);
    mat4_identity(view->view_matrix);
    mat4_rotate_X(view->view_matrix, view->view_matrix, (-M_PI / 2) - dir[1]);
    mat4_rotate_Z(view->view_matrix, view->view_matrix, (M_PI / 2) - dir[0]);
    mat4_translate_in_place(view->view_matrix, -pos[0], -pos[1], -pos[2]);
}

void pg_viewer_project(struct pg_viewer* view, vec2 out, vec3 const pos)
{
    vec4 pos_ = { pos[0], pos[1], pos[2], 1 };
    vec4 out_;
    mat4_mul_vec4(out_, view->view_matrix, pos_);
    mat4_mul_vec4(out_, view->proj_matrix, out_);
    out_[0] /= out_[3];
    out_[0] = out_[0] * 0.5 + 0.5;
    out_[0] *= view->size[0] / view->size[1];
    out_[1] /= out_[3];
    out_[1] = -out_[1] * 0.5 + 0.5;
    vec2_dup(out, out_);
}

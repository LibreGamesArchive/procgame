#include "../linmath.h"
#include "../procgl/procgl.h"
#include "../shaders/shaders.h"
#include "game.h"

static void collider_generate_ring_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            float _x = x - 64;
            float _y = y - 64;
            float dist = sqrt(_x * _x + _y * _y) - 44;
            if(dist < 0 || dist >= 16) continue;
            dist -= 8;
            float r_dist = 1 - fabs(dist) / 8;
            uint8_t c = r_dist * 128 + 100;
            tex->pixels[x + y * tex->w] =
                (struct pg_texture_pixel) { c * 0.5, c * 0.5, c, 255 };
            tex->normals[x + y * tex->w].h = r_dist * 255;
        }
    }
    pg_texture_generate_normals(tex);
    pg_texture_buffer(tex);
}

void collider_init(struct collider_state* coll)
{
    collider_generate_ring_texture(&coll->ring_texture);
    pg_shader_3d(&coll->shader_3d);
    pg_shader_3d_set_texture(&coll->shader_3d, &coll->ring_texture, 0, 1);
    pg_shader_3d_set_fog(&coll->shader_3d,
                         (vec2){ 18, 20 }, (vec3){ 0.67, 0.58, 0.42 });
    pg_shader_3d_set_light(&coll->shader_3d,
                           (vec3){ 0.2, 0.2, 0.2 },
                           (vec3){ 0, 0, -1 },
                           (vec3){ 3, 3, 3 });
    pg_model_quad(&coll->ring_model);
    pg_model_precalc_verts(&coll->ring_model);
    pg_model_buffer(&coll->ring_model, &coll->shader_3d);
    pg_viewer_init(&coll->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
                   (vec2){ 800, 600 }, (vec2){ 0.1, 100 });
    coll->player_angle = 0.3;
    vec2_set(coll->player_pos, 0, 0);
    ARR_INIT(coll->rings);
    int i;
    for(i = 0; i < 12; ++i) {
        struct coll_ring r =
            { (M_PI * 2) / 12 * i, { (i - 6) * 0.25, (i - 6) * 0.25 } };
        ARR_PUSH(coll->rings, r);
    }
}

void collider_deinit(struct collider_state* coll)
{
    pg_shader_deinit(&coll->shader_3d);
    pg_model_deinit(&coll->ring_model);
    pg_texture_deinit(&coll->ring_texture);
    ARR_DEINIT(coll->rings);
}

void collider_update(struct collider_state* coll)
{
    int mouse_x, mouse_y;
    SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
    /*  Handle input; get the current view angle, add mouse motion to it  */
    #if 1
    coll->player_pos[0] += mouse_x * 0.001;
    coll->player_pos[1] += mouse_y * 0.001;
    pg_viewer_set(&coll->view,
        (vec3){ (10 + coll->player_pos[0]) * cos(coll->player_angle),
                (10 + coll->player_pos[0]) * sin(coll->player_angle),
                coll->player_pos[1] },
        (vec2){ coll->player_angle - M_PI / 2, 0 });
    #else
    pg_viewer_set(&coll->view, (vec3){ 1, 0, 0 },
        (vec2){ coll->view.dir[0] - mouse_x * 0.001, coll->view.dir[1] - mouse_y * 0.001 });
    #endif
    coll->player_angle += 0.01;
}

void collider_draw(struct collider_state* coll)
{
    pg_shader_begin(&coll->shader_3d, &coll->view);
    pg_model_begin(&coll->ring_model);
    struct coll_ring* r;
    int i;
    ARR_FOREACH_PTR(coll->rings, r, i) {
        mat4 model_transform;
        mat4_translate(model_transform,
            (10 + r->pos[0]) * cos(r->angle),
            (10 + r->pos[0]) * sin(r->angle), r->pos[1]);
        mat4_rotate_Z(model_transform, model_transform, r->angle);
        pg_model_draw(&coll->ring_model, &coll->shader_3d, model_transform);
    }
}

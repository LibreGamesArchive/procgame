#include "../linmath.h"
#include "../procgl/procgl.h"
#include "../shaders/shaders.h"
#include "game.h"
#define GAME_WIDTH 16
#define GAME_RADIUS 80
#define GAME_SEGMENTS 30

static void collider_generate_ring_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128, 0, 1);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            float _x = x - 64;
            float _y = y - 64;
            float dist = sqrt(_x * _x + _y * _y) - 44;
            if(dist < 0 || dist >= 16) {
                tex->pixels[x + y * tex->w] =
                    (struct pg_texture_pixel){ 50, 50, 100, 0 };
                continue;
            }
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

static void collider_generate_ring_model(struct pg_model* model,
                                         struct pg_shader* shader)
{
    pg_model_quad(model);
    pg_model_precalc_verts(model);
    pg_model_buffer(model, shader);
}

static void collider_generate_env_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128, 2, 3);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            struct pg_texture_pixel* p = &tex->pixels[x + y * tex->w];
            struct pg_texture_normal* n = &tex->normals[x + y * tex->w];
            float _x = x - 64;
            float _y = y - 64;
            float dist = sqrt(_x * _x + _y * _y);
            if(dist >= 50) {
                float s = fabs(sin(y / (128 / (M_PI * 2)) * 8));
                s = abs((y % 8) - 4);
                float c = s * 16 + 64;
                *p = (struct pg_texture_pixel){ 128, 128, 128, 255 };
                n->h = pow(s, 3) * 3;
            } else if(dist >= 35) {
                float s = pow(8 - fabs(dist - 43), 2) + 60;
                *p = (struct pg_texture_pixel){ s * 2, s * 2, s * 2, 255 };
                n->h = 255 - (s * 2);
            } else {
                float c = dist * 3;
                *p = (struct pg_texture_pixel){ c * 0.25, c * 0.25, c, 255 };
                n->h = dist * 10;
            }
        }
    }
    pg_texture_generate_normals(tex);
    pg_texture_buffer(tex);
}

static void collider_generate_env_model(struct pg_model* model,
                                        struct pg_shader* shader)
{
    float half_angle = M_PI / GAME_SEGMENTS;
    pg_model_init(model);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 0, 1, 2);
    pg_model_add_triangle(model, 2, 1, 3);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 5, 4, 6);
    pg_model_add_triangle(model, 5, 6, 7);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 9, 8, 10);
    pg_model_add_triangle(model, 9, 10, 11);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 12, 13, 14);
    pg_model_add_triangle(model, 14, 13, 15);
    pg_model_precalc_verts(model);
    pg_model_buffer(model, shader);
}

void collider_init(struct collider_state* coll)
{
    pg_gbuffer_init(&coll->gbuf, 800, 600);
    pg_gbuffer_bind(&coll->gbuf, 16, 17, 18, 19);
    collider_generate_ring_texture(&coll->ring_texture);
    collider_generate_env_texture(&coll->env_texture);
    pg_texture_init_from_file(&coll->font, "font_8x8.png", NULL, 8, -1);
    pg_texture_set_atlas(&coll->font, 8, 8);
    pg_shader_text(&coll->shader_text);
    pg_shader_text_set_font(&coll->shader_text, &coll->font);
    pg_shader_3d(&coll->shader_3d);
    collider_generate_ring_model(&coll->ring_model, &coll->shader_3d);
    collider_generate_env_model(&coll->env_model, &coll->shader_3d);
    coll->player_angle = 0.3;
    coll->player_speed = 0.1;
    coll->player_light_intensity = 10;
    vec2_set(coll->player_pos, 0, 0);
    pg_viewer_init(&coll->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
                   (vec2){ 800, 600 }, (vec2){ 0.1, 100 });
    ARR_INIT(coll->rings);
    int i;
    for(i = 0; i < 12; ++i) {
        struct coll_ring r =
            { 0.5, (M_PI * 2) / 12 * i, { (i - 6) * 0.25, (i - 6) * 0.25 } };
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

/*  This function should approach 1 as x approaches infinity
    1 here represents the speed of light    */
static float speed_func(float x)
{
    return (x / (x + 1.5)) * 0.1;
}

void collider_update(struct collider_state* coll)
{
    int mouse_x, mouse_y;
    SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
    coll->player_pos[0] += mouse_x * 0.005;
    coll->player_pos[1] -= mouse_y * 0.005;
    pg_viewer_set(&coll->view,
        (vec3){ (GAME_RADIUS + coll->player_pos[0]) * cos(coll->player_angle),
                (GAME_RADIUS + coll->player_pos[0]) * sin(coll->player_angle),
                coll->player_pos[1] },
        (vec2){ coll->player_angle + M_PI / 2, 0 });
    float new_angle = coll->player_angle + speed_func(coll->player_speed);
    struct coll_ring* r;
    int i;
    ARR_FOREACH_PTR_REV(coll->rings, r, i) {
        if((new_angle >= M_PI * 2 && fmod(new_angle, M_PI * 2) >= r->angle)
        || (coll->player_angle < r->angle && new_angle >= r->angle)) {
            vec2 diff;
            vec2_sub(diff, coll->player_pos, r->pos);
            float dist = vec2_len(diff);
            float radius = (9 - (r->power * 8)) * 0.5;
            if(dist < radius) {
                coll->player_speed += r->power * 0.1;
                coll->player_light_intensity += 20;
                ARR_SWAPSPLICE(coll->rings, i, 1);
            }
        }
    }
    coll->player_angle += speed_func(coll->player_speed);
    coll->player_angle = fmod(coll->player_angle, M_PI * 2);
    coll->player_light_intensity *= 0.99;
}

void collider_draw(struct collider_state* coll)
{
    pg_gbuffer_dst(&coll->gbuf);
    /*  All of this renders to the gbuffer, for lighting later  */
    pg_shader_begin(&coll->shader_3d, &coll->view);
    pg_shader_3d_set_texture(&coll->shader_3d, 2, 3);
    pg_model_begin(&coll->env_model);
    int i;
    for(i = 0; i < GAME_SEGMENTS; ++i) {
        mat4 model_transform;
        mat4_identity(model_transform);
        mat4_rotate_Z(model_transform, model_transform, (M_PI * 2) / GAME_SEGMENTS * i);
        pg_model_draw(&coll->env_model, &coll->shader_3d, model_transform);
    }
    pg_shader_3d_set_texture(&coll->shader_3d, 0, 1);
    pg_model_begin(&coll->ring_model);
    struct coll_ring* r;
    ARR_FOREACH_PTR(coll->rings, r, i) {
        float radius = (9 - (r->power * 8)) * 0.5;
        mat4 model_transform;
        mat4_translate(model_transform,
            (GAME_RADIUS + r->pos[0]) * cos(r->angle),
            (GAME_RADIUS + r->pos[0]) * sin(r->angle), r->pos[1]);
        mat4_rotate_Z(model_transform, model_transform, r->angle);
        mat4_scale_aniso(model_transform, model_transform, radius, radius, radius);
        pg_model_draw(&coll->ring_model, &coll->shader_3d, model_transform);
    }
    /*  Now we start drawing all the lights */
    pg_gbuffer_begin_light(&coll->gbuf, &coll->view);
    ARR_FOREACH_PTR(coll->rings, r, i) {
        float angle = r->angle - 0.1;
        pg_gbuffer_draw_light(&coll->gbuf,
            (vec4){ (GAME_RADIUS) * cos(angle),
                    (GAME_RADIUS) * sin(angle), 3, 15 },
            (vec3){ 1, 0.5, 0.5 });
    }
    pg_gbuffer_draw_light(&coll->gbuf,
        (vec4){ coll->view.pos[0], coll->view.pos[1], coll->view.pos[2],
                coll->player_light_intensity },
        (vec3){ 1, 1, 1 });
    /*  And finish directly to the screen, with a tiny bit of ambient light */
    pg_screen_dst();
    pg_gbuffer_finish(&coll->gbuf, (vec3){ 0.1, 0.1, 0.1 });
    pg_shader_begin(&coll->shader_text, &coll->view);
    char speed_str[10];
    snprintf(speed_str, 10, "%.5f c", speed_func(coll->player_speed));
    pg_shader_text_write(&coll->shader_text, speed_str,
        (vec2){ 32, 500 }, (vec2){ 24, 24 }, 0.25);
}

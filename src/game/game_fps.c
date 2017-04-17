#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/ext/noise1234.h"

struct fps_game_renderer {
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    struct pg_shader shader_3d;
    struct pg_shader shader_text;
};

struct fps_game_assets {
    struct pg_model floor_model;
    struct pg_model test_cyl;
    struct pg_model test_cone;
    struct pg_model test_cone_trunc;
    struct pg_texture floor_tex;
    struct pg_texture font;
    struct pg_sdf_tree test_sdf;
};

struct fps_game_data {
    struct fps_game_renderer rend;
    struct fps_game_assets assets;
    const uint8_t* kb_state;
    vec2 mouse_motion;
    vec3 player_pos, player_vel;
    vec2 player_dir;
};

static void fps_game_generate_assets(struct fps_game_data* d);

static void fps_game_update(struct pg_game_state* state);
static void fps_game_tick(struct pg_game_state* state);
static void fps_game_draw(struct pg_game_state* state);
static void fps_game_deinit(void* data);

void fps_game_start(struct pg_game_state* state)
{
    pg_game_state_init(state, pg_time(), 60, 3);
    struct fps_game_data* d = malloc(sizeof(*d));
    int sw, sh;
    pg_screen_size(&sw, &sh);
    pg_gbuffer_init(&d->rend.gbuf, sw, sh);
    pg_gbuffer_bind(&d->rend.gbuf, 20, 21, 22, 23);
    pg_viewer_init(&d->rend.view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        (vec2){ sw, sh }, (vec2){ 0.1, 200});
    pg_shader_3d(&d->rend.shader_3d);
    pg_shader_text(&d->rend.shader_text);
    fps_game_generate_assets(d);
    pg_shader_3d_set_texture(&d->rend.shader_3d, &d->assets.floor_tex);
    pg_shader_text_set_font(&d->rend.shader_text, &d->assets.font);
    d->kb_state = SDL_GetKeyboardState(NULL);
    vec2_set(d->mouse_motion, 0, 0);
    vec2_set(d->player_dir, 0, 0);
    vec3_set(d->player_pos, 0, 0, 1);
    vec3_set(d->player_vel, 0, 0, 0);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    state->data = d;
    state->update = fps_game_update;
    state->tick = fps_game_tick;
    state->draw = fps_game_draw;
    state->deinit = fps_game_deinit;
}

static void fps_game_update(struct pg_game_state* state)
{
    struct fps_game_data* d = state->data;
    int mx, my;
    SDL_GetRelativeMouseState(&mx, &my);
    d->mouse_motion[0] -= mx * 0.0005f;
    d->mouse_motion[1] -= my * 0.0005f;
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->tick = NULL;
    }
    if(d->kb_state[SDL_SCANCODE_A]) {
        d->player_vel[0] -= 0.1 * sin(d->player_dir[0]);
        d->player_vel[1] += 0.1 * cos(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_D]) {
        d->player_vel[0] += 0.1 * sin(d->player_dir[0]);
        d->player_vel[1] -= 0.1 * cos(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_W]) {
        d->player_vel[0] += 0.1 * cos(d->player_dir[0]);
        d->player_vel[1] += 0.1 * sin(d->player_dir[0]);
    }
    if(d->kb_state[SDL_SCANCODE_S]) {
        d->player_vel[0] -= 0.1 * cos(d->player_dir[0]);
        d->player_vel[1] -= 0.1 * sin(d->player_dir[0]);
    }
}

static void fps_game_tick(struct pg_game_state* state)
{
    struct fps_game_data* d = state->data;
    vec2_set(d->player_dir,
             d->player_dir[0] + d->mouse_motion[0],
             d->player_dir[1] + d->mouse_motion[1]);
    vec2_set(d->mouse_motion, 0, 0);
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
}

static void fps_game_draw(struct pg_game_state* state)
{
    struct fps_game_data* d = state->data;
    /*  Interpolation   */
    float t = state->tick_over;
    vec2 vel_lerp = { d->player_vel[0] * t, d->player_vel[1] * t };
    vec3 pos = { d->player_pos[0] + vel_lerp[0],
                 d->player_pos[1] + vel_lerp[1], 2 };
    vec2 dir = { d->player_dir[0] + d->mouse_motion[0],
                 d->player_dir[1] + d->mouse_motion[1] };
    pg_viewer_set(&d->rend.view, pos, dir);
    /*  Drawing */
    pg_gbuffer_dst(&d->rend.gbuf);
    pg_shader_begin(&d->rend.shader_3d, &d->rend.view);
    pg_model_begin(&d->assets.floor_model, &d->rend.shader_3d);
    mat4 model_transform;
    mat4_identity(model_transform);
    pg_model_draw(&d->assets.floor_model, model_transform);
    pg_model_begin(&d->assets.test_cyl, &d->rend.shader_3d);
    mat4_translate(model_transform, 4, 0, 0);
    mat4_rotate_Z(model_transform, model_transform, (float)state->ticks * 0.01);
    pg_model_draw(&d->assets.test_cyl, model_transform);
    /*  Lighting    */
    pg_gbuffer_begin_light(&d->rend.gbuf, &d->rend.view);
    pg_gbuffer_draw_light(&d->rend.gbuf,
        (vec4){ 0, 0, 0.25, 5 },
        (vec3){ 1, 0.25, 0.25 });
    pg_screen_dst();
    pg_gbuffer_finish(&d->rend.gbuf, (vec3){ 0.3, 0.3, 0.3 });
    /*  Overlay */
    pg_shader_begin(&d->rend.shader_text, NULL);
    char fps_str[10];
    snprintf(fps_str, 10, "FPS: %d", (int)pg_framerate());
    pg_shader_text_write(&d->rend.shader_text, fps_str,
        (vec2){ 0, 0 }, (vec2){ 8, 8 }, 0.1);
}

static void fps_game_deinit(void* data)
{
    struct fps_game_data* d = data;
    pg_shader_deinit(&d->rend.shader_3d);
    pg_gbuffer_deinit(&d->rend.gbuf);
    pg_model_deinit(&d->assets.floor_model);
    pg_texture_deinit(&d->assets.floor_tex);
    free(d);
}

static void fps_game_floor_texture_sdf(struct pg_texture* tex,
                                       struct pg_sdf_tree* tree,
                                       mat4 transform)
{
    float h[128 * 128];
    struct pg_heightmap hmap = { .map = h, .w = 128, .h = 128 };
    pg_texture_init(tex, 128, 128);
    pg_texture_bind(tex, 1, 2);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            vec4 p = { (float)x / 128 * 2 - 1,
                       (float)y / 128 * 2 - 1, 0 , 1 };
            //mat4_mul_vec4(p, transform, p);
            float s = pg_sdf_tree_sample(tree, p);
            float d_mod = fmod(s * 2, 1);
            float line = powf(MAX(d_mod, 1 - d_mod), 2);
            pg_texel_set(tex->diffuse[x + y * 128],
                line * 250.0f, line * 250.0f, line * 250.0f, 250);
            hmap.map[x + y * 128] = line;
        }
    }
    pg_texture_generate_normals(tex, &hmap, 2);
    pg_texture_buffer(tex);
}

static void fps_game_generate_floor_texture(struct pg_texture* tex)
{
    float h[128 * 128];
    struct pg_heightmap hmap = { .map = h, .w = 128, .h = 128 };
    pg_texture_init(tex, 128, 128);
    pg_texture_bind(tex, 1, 2);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            float _x = fabs(x - 64);
            float _y = fabs(y - 64);
            float dist = (_x > _y) ? _x : _y;
            float s = noise2(x / 8.0f, y / 8.0f) * 0.5 + 0.5;
            float c = s * 32 + 150;
            if(dist < 56) {
                hmap.map[x + y * 128] = s;
                pg_texel_set(tex->diffuse[x + y * 128], c, c, c, 255);
                pg_texel_set(tex->light[x + y * 128], 0, 0, 0, s * 48 + 128);
            } else {
                float scale = 1 - ((dist - 56) / 8);
                c *= scale * 0.5 + 0.5;
                s *= scale;
                hmap.map[x + y * 128] = s;
                pg_texel_set(tex->diffuse[x + y * 128], c, c, c, 255);
                pg_texel_set(tex->light[x + y * 128], 0, 0, 0, s * 48);
            }
        }
    }
    pg_texture_generate_normals(tex, &hmap, 2);
    pg_texture_buffer(tex);
}

static void fps_game_generate_assets(struct fps_game_data* d)
{
    /*  Generating the floor model  */
    pg_model_init(&d->assets.floor_model);
    pg_model_quad(&d->assets.floor_model, (vec2){ 10, 10 });
    pg_model_precalc_ntb(&d->assets.floor_model);
    mat4 transform; 
    mat4_identity(transform);
    mat4_rotate_X(transform, transform, -M_PI / 2);
    mat4_scale(transform, transform, 10);
    pg_model_transform(&d->assets.floor_model, transform);
    pg_shader_buffer_model(&d->rend.shader_3d, &d->assets.floor_model);
    // Test cylinder
    pg_model_init(&d->assets.test_cyl);
    pg_model_cylinder(&d->assets.test_cyl, 16, (vec2){ 6, 6 });
    pg_model_precalc_ntb(&d->assets.test_cyl);
    pg_model_blend_duplicates(&d->assets.test_cyl, 0.8);
    mat4_identity(transform);
    mat4_scale_aniso(transform, transform, 1, 1, 4);
    mat4_translate_in_place(transform, 0, 0, 0);
    pg_model_transform(&d->assets.test_cyl, transform);
    pg_shader_buffer_model(&d->rend.shader_3d, &d->assets.test_cyl);
    /*  Generating the SDF tree */
    char sdf_src[] = "(union (BOX 0.5 1 1) (BOX 1 0.5 0.5))";
    pg_sdf_tree_init(&d->assets.test_sdf);
    pg_sdf_tree_parse(&d->assets.test_sdf, sdf_src, sizeof(sdf_src));
    /*  Generating the floor texture    */
    fps_game_generate_floor_texture(&d->assets.floor_tex);
    mat4_identity(transform);
    //fps_game_floor_texture_sdf(&d->assets.floor_tex, &d->assets.test_sdf, transform);
    /*  Loading the font texture    */
    pg_texture_init_from_file(&d->assets.font, "font_8x8.png", NULL);
    pg_texture_set_atlas(&d->assets.font, 8, 8);
    pg_texture_bind(&d->assets.font, 3, 4);
}



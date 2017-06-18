#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/wave_defs.h"
#include "BORK_of_DOOM/map_area.h"

struct bork_game_renderer {
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    struct pg_shader shader_3d;
    struct pg_shader shader_text;
};

struct bork_game_assets {
    struct pg_model map_model;
    struct pg_texture env_atlas;
    struct pg_texture font;
};

struct bork_game_data {
    struct bork_game_renderer rend;
    struct bork_game_assets assets;
    struct bork_map map;
    const uint8_t* kb_state;
    vec2 mouse_motion;
    vec3 player_pos, player_vel;
    vec2 player_dir;
};

static void bork_game_update(struct pg_game_state* state);
static void bork_game_tick(struct pg_game_state* state);
static void bork_game_draw(struct pg_game_state* state);
static void bork_game_deinit(void* data);

static void bork_game_generate_map(struct bork_game_data* d, int w, int h, int l);
static void bork_game_generate_assets(struct bork_game_data* d);

void bork_game_start(struct pg_game_state* state)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 3);
    struct bork_game_data* d = malloc(sizeof(*d));
    d->kb_state = SDL_GetKeyboardState(NULL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    vec2_set(d->mouse_motion, 0, 0);
    /*  Set up the gbuffer for deferred shading */
    int sw, sh;
    pg_screen_size(&sw, &sh);
    pg_gbuffer_init(&d->rend.gbuf, sw, sh);
    pg_gbuffer_bind(&d->rend.gbuf, 20, 21, 22, 23);
    pg_viewer_init(&d->rend.view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        (vec2){ sw, sh }, (vec2){ 0.1, 200});
    /*  Set up the shaders  */
    pg_shader_3d(&d->rend.shader_3d);
    pg_shader_text(&d->rend.shader_text);
    /*  Generate the models, textures, sounds, etc.*    */
    bork_game_generate_assets(d);
    /*  Attach the font texture to the text shader  */
    pg_shader_text_set_font(&d->rend.shader_text, &d->assets.font);
    pg_shader_3d_set_texture(&d->rend.shader_3d, &d->assets.env_atlas);
    /*  Initialize the player data  */
    vec2_set(d->player_dir, 0, 0);
    vec3_set(d->player_pos, 0, 0, 1);
    vec3_set(d->player_vel, 0, 0, 0);
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = bork_game_update;
    state->tick = bork_game_tick;
    state->draw = bork_game_draw;
    state->deinit = bork_game_deinit;
}

static void bork_game_update(struct pg_game_state* state)
{
    struct bork_game_data* d = state->data;
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

static void bork_game_tick(struct pg_game_state* state)
{
    struct bork_game_data* d = state->data;
    vec2_set(d->player_dir,
             d->player_dir[0] + d->mouse_motion[0],
             d->player_dir[1] + d->mouse_motion[1]);
    vec2_set(d->mouse_motion, 0, 0);
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
}

static void bork_game_draw(struct pg_game_state* state)
{
    struct bork_game_data* d = state->data;
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
    pg_model_begin(&d->assets.map_model, &d->rend.shader_3d);
    mat4 model_transform;
    mat4_identity(model_transform);
    pg_model_draw(&d->assets.map_model, model_transform);
    /*  Lighting    */
    pg_gbuffer_begin_light(&d->rend.gbuf, &d->rend.view);
    pg_gbuffer_draw_light(&d->rend.gbuf,
        (vec4){ pos[0], pos[1], pos[2], 10 },
        (vec3){ 1, 1, 1 });
    pg_screen_dst();
    pg_gbuffer_finish(&d->rend.gbuf, (vec3){ 0.025, 0.025, 0.025 });
    /*  Overlay */
    pg_shader_begin(&d->rend.shader_text, NULL);
    char bork_str[10];
    snprintf(bork_str, 10, "FPS: %d", (int)pg_framerate());
    pg_shader_text_write(&d->rend.shader_text, bork_str,
        (vec2){ 0, 0 }, (vec2){ 8, 8 }, 0.25);
}

static void bork_game_deinit(void* data)
{
    struct bork_game_data* d = data;
    pg_shader_deinit(&d->rend.shader_3d);
    pg_gbuffer_deinit(&d->rend.gbuf);
    free(d);
}

static void bork_game_floor_texture_sdf(struct pg_texture* tex,
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

static void bork_game_generate_floor_texture(struct pg_texture* tex)
{
    /*  Define a wave function: this is two octaves of perlin noise with the
        "seamless 2d" modifier, it should generate a vaguely rough sort of
        rocky texture which repeats seamlessly in both directions.    */
    struct pg_wave w[] = {
        { PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_SEAMLESS_2D },
        { PG_WAVE_MODIFIER, .mod = PG_WAVE_MOD_OCTAVES,
            .octaves = 2, .ratio = 2, .decay = 0.5 },
        PG_WAVE_PERLIN(.scale = 1)
    };
    float h[128 * 128];
    struct pg_heightmap hmap = { .map = h, .w = 128, .h = 128 };
    pg_heightmap_from_wave(&hmap, &PG_WAVE_ARRAY(w, 3), 1, 1);
    pg_texture_init(tex, 128, 128);
    pg_texture_bind(tex, 1, 2);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            float h_ = h[x + y * 128] * 0.5 + 0.5;
            pg_texel_set(tex->diffuse[x + y * 128],
                h_ * 128 + 64, h_ * 64 + 32, h_ * 32 + 16, 255);
            pg_texel_set(tex->light[x + y * 128], 0, 0, 0, 0);
        }
    }
    pg_texture_generate_normals(tex, &hmap, 10);
    pg_texture_buffer(tex);
}

static void bork_game_generate_assets(struct bork_game_data* d)
{
    /*  Loading the font texture    */
    pg_texture_init_from_file(&d->assets.font, "font_8x8.png", NULL);
    pg_texture_set_atlas(&d->assets.font, 8, 8);
    pg_texture_bind(&d->assets.font, 3, 4);
    pg_texture_init_from_file(&d->assets.env_atlas, "res/env_atlas.png", "res/env_atlas_lightmap.png");
    pg_texture_set_atlas(&d->assets.env_atlas, 32, 32);
    pg_texture_bind(&d->assets.env_atlas, 5, 6);
    pg_model_init(&d->assets.map_model);
    bork_game_generate_map(d, 32, 32, 128);
}


static void bork_game_generate_map(struct bork_game_data* d, int w, int l, int h)
{
    bork_map_init(&d->map, w, l, h);
    int x, y, z;
    for(x = 0; x < w; ++x) {
        for(y = 0; y < l; ++y) {
            for(z = 0; z < h; ++z) {
                if((rand() % 20 == 0) || (z == 12) || (x < 8 && (z == 0)) || (x >= 8 && z <= x - 8)) {
                    bork_map_set_tile(&d->map, x, y, z, (struct bork_tile) {
                        BORK_TILE_HULL });
                } else {
                    bork_map_set_tile(&d->map, x, y, z, (struct bork_tile) {
                        BORK_TILE_VAC });
                }
            }
        }
    }
    bork_map_generate_model(&d->map, &d->assets.map_model, &d->assets.env_atlas);
    pg_shader_buffer_model(&d->rend.shader_3d, &d->assets.map_model);
}

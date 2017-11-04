#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

#define TINYFILES_IMPL
#include "tinyfiles.h"

void bork_read_saves(struct bork_game_core* core);

void bork_init(struct bork_game_core* core, char* base_path)
{
    srand(time(NULL));
    int sw, sh;
    pg_screen_size(&sw, &sh);
    *core = (struct bork_game_core) {
        .screen_size = { sw, sh },
        .aspect_ratio = (float)sw / (float)sh,
        .ctrl_map = {
            [BORK_CTRL_UP] = SDL_SCANCODE_W,
            [BORK_CTRL_DOWN] = SDL_SCANCODE_S,
            [BORK_CTRL_LEFT] = SDL_SCANCODE_A,
            [BORK_CTRL_RIGHT] = SDL_SCANCODE_D,
            [BORK_CTRL_JUMP] = SDL_SCANCODE_SPACE,
            [BORK_CTRL_FIRE] = PG_LEFT_MOUSE,
            [BORK_CTRL_SELECT] = SDL_SCANCODE_E,
            [BORK_CTRL_ESCAPE] = SDL_SCANCODE_ESCAPE,
            [BORK_CTRL_BIND1] = SDL_SCANCODE_1,
            [BORK_CTRL_BIND2] = SDL_SCANCODE_2,
            [BORK_CTRL_BIND3] = SDL_SCANCODE_3,
            [BORK_CTRL_BIND4] = SDL_SCANCODE_4,
        },
    };
    if(base_path) {
        core->base_path = base_path;
        core->base_path_len = strlen(core->base_path);
        core->free_base_path = 1;
    } else {
        core->base_path = "./";
        core->base_path_len = 2;
        core->free_base_path = 0;
    }
    /*  Set up the gbuffer for deferred shading */
    pg_gbuffer_init(&core->gbuf, sw, sh);
    pg_gbuffer_bind(&core->gbuf, 21, 22, 23, 24);
    pg_viewer_init(&core->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        core->screen_size, (vec2){ 0.01, 200 });
    /*  Set up the shaders  */
    pg_shader_3d(&core->shader_3d);
    pg_shader_2d(&core->shader_2d);
    pg_shader_sprite(&core->shader_sprite);
    pg_shader_text(&core->shader_text);
    pg_ppbuffer_init(&core->ppbuf, sw, sh, 26, 27);
    pg_postproc_blur(&core->post_blur, PG_BLUR7);
    pg_postproc_screen(&core->post_screen);
    /*  Get the models, textures, sounds, etc.*    */
    bork_load_assets(core);
    /*  Attach the font texture to the text shader  */
    pg_shader_text_font(&core->shader_text, &core->font);
    pg_shader_3d_texture(&core->shader_3d, &core->env_atlas);
    pg_shader_2d_texture(&core->shader_2d, &core->editor_atlas);
    pg_shader_sprite_texture(&core->shader_sprite, &core->bullet_tex);
    pg_shader_sprite_tex_frame(&core->shader_sprite, 0);
    pg_gamepad_config(0.125, 0.6, 0.125, 0.75);
    if(SDL_NumJoysticks()) pg_use_gamepad(0);
    bork_read_saves(core);
}

static void backdrop_color_func(vec4 out, vec2 p, struct pg_wave* wave)
{
    float s = pg_wave_sample(wave, 2, p);
    vec4_set(out, s, s, s, 1);
}

static void vignette_color_func(vec4 out, vec2 p, struct pg_wave* wave)
{
    float s = pg_wave_sample(wave, 2, p);
    vec4_set(out, 1, 1, 1, s);
}

static void load_from_base_dir(struct bork_game_core* core,
                               struct pg_texture* tex, char* f1, char* f2)
{
    char f1_[1024];
    char f2_[1024];
    if(f1) snprintf(f1_, 1024, "%s%s", core->base_path, f1);
    if(f2) snprintf(f2_, 1024, "%s%s", core->base_path, f2);
    pg_texture_init_from_file(tex, f1 ? f1_ : NULL, f2 ? f2_ : NULL);
}

void bork_load_assets(struct bork_game_core* core)
{
    /*  Loading the textures and setting the atlas dimensions   */
    load_from_base_dir(core, &core->font, "font_8x8.png", NULL);
    pg_texture_set_atlas(&core->font, 8, 8);
    pg_texture_bind(&core->font, 3, 4);
    load_from_base_dir(core, &core->env_atlas, "res/env_atlas.png", "res/env_atlas_lightmap.png");
    pg_texture_set_atlas(&core->env_atlas, 32, 32);
    pg_texture_bind(&core->env_atlas, 5, 6);
    load_from_base_dir(core, &core->editor_atlas, "res/editor_atlas.png", NULL);
    pg_texture_set_atlas(&core->editor_atlas, 32, 32);
    pg_texture_bind(&core->editor_atlas, 7, 0);
    load_from_base_dir(core, &core->bullet_tex, "res/bullets.png", "res/bullets_lightmap.png");
    pg_texture_set_atlas(&core->bullet_tex, 16, 16);
    pg_texture_bind(&core->bullet_tex, 8, 9);
    load_from_base_dir(core, &core->item_tex, "res/items.png", "res/items_lightmap.png");
    pg_texture_set_atlas(&core->item_tex, 16, 16);
    pg_texture_bind(&core->item_tex, 10, 11);
    load_from_base_dir(core, &core->enemies_tex, "res/enemies.png", "res/enemies_lightmap.png");
    pg_texture_set_atlas(&core->enemies_tex, 32, 32);
    pg_texture_bind(&core->enemies_tex, 12, 13);
    load_from_base_dir(core, &core->particle_tex, "res/particles.png", "res/particles_lightmap.png");
    pg_texture_set_atlas(&core->particle_tex, 16, 16);
    pg_texture_bind(&core->particle_tex, 14, 15);
    load_from_base_dir(core, &core->upgrades_tex, "res/upgrades.png", NULL);
    pg_texture_set_atlas(&core->upgrades_tex, 32, 32);
    pg_texture_bind(&core->upgrades_tex, 16, 0);
    /*  Generate the backdrop texture (cloudy reddish fog)  */
    pg_texture_init(&core->backdrop_tex, 256, 256);
    pg_texture_init(&core->menu_vignette, 256, 256);
    pg_texture_init(&core->radial_vignette, 256, 256);
    pg_texture_bind(&core->backdrop_tex, 18, -1);
    pg_texture_bind(&core->menu_vignette, 19, -1);
    pg_texture_bind(&core->radial_vignette, 20, -1);
    float seed = (float)rand() / RAND_MAX * 1000;
    struct pg_wave backdrop_wave[8] = {
        PG_WAVE_MOD_SEAMLESS_2D(.scale = 0.5),
        PG_WAVE_MOD_OCTAVES(.octaves = 5, .ratio = 3, .decay = 0.5),
        PG_WAVE_FUNC_PERLIN(.frequency = { 4, 4, 4, 4 }, .phase = { seed }),
    };
    struct pg_wave vignette_wave[8] = {
        PG_WAVE_FUNC_MAX(.frequency = {0.5, 1.4}, .scale = 10, .add = -9),
    };
    struct pg_wave radial_vignette_wave[8] = {
        PG_WAVE_FUNC_DISTANCE()
    };
    pg_texture_wave_to_colors(&core->backdrop_tex, &PG_WAVE_ARRAY(backdrop_wave, 8),
                              backdrop_color_func);
    pg_texture_wave_to_colors(&core->menu_vignette, &PG_WAVE_ARRAY(vignette_wave, 8),
                              vignette_color_func);
    pg_texture_wave_to_colors(&core->radial_vignette, &PG_WAVE_ARRAY(radial_vignette_wave, 8),
                              vignette_color_func);
    pg_texture_buffer(&core->backdrop_tex);
    pg_texture_buffer(&core->menu_vignette);
    pg_texture_buffer(&core->radial_vignette);
    /*  Generate the basic models, just quads for now   */
    mat4 transform;
    mat4_identity(transform);
    /*  Rotate the 3d sprites upward to point along the Y axis instead of Z */
    mat4_rotate_X(transform, transform, M_PI * 0.5);
    mat4_scale(transform, transform, 2);
    pg_model_init(&core->enemy_model);
    pg_model_quad(&core->enemy_model, (vec2){ 1, 1 });
    pg_model_transform(&core->enemy_model, transform);
    pg_shader_buffer_model(&core->shader_sprite, &core->enemy_model);
    mat4_scale(transform, transform, 0.25);
    pg_model_init(&core->bullet_model);
    pg_model_quad(&core->bullet_model, (vec2){ 1, 1 });
    pg_model_transform(&core->bullet_model, transform);
    pg_shader_buffer_model(&core->shader_sprite, &core->bullet_model);
    mat4_scale_aniso(transform, transform, 1, 1, 0.5);
    pg_model_init(&core->gun_model);
    pg_model_quad(&core->gun_model, (vec2){ 1, 1 });
    pg_model_transform(&core->gun_model, transform);
    pg_model_precalc_ntb(&core->gun_model);
    pg_shader_buffer_model(&core->shader_3d, &core->gun_model);
    /*  Our basic 2d quad for drawing images (centered)  */
    pg_model_init(&core->quad_2d_ctr);
    pg_model_quad(&core->quad_2d_ctr, (vec2){ 1, 1 });
    mat4_identity(transform);
    mat4_scale_aniso(transform, transform, 2, -2, 0);
    pg_model_transform(&core->quad_2d_ctr, transform);
    pg_model_reserve_component(&core->quad_2d_ctr,
        PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    vec4_t* color;
    float* f;
    int i;
    ARR_FOREACH_PTR(core->quad_2d_ctr.height, f, i) {
        *f = 1.0f;
    }
    ARR_FOREACH_PTR(core->quad_2d_ctr.color, color, i) {
        vec4_set(color->v, 1, 1, 1, 1);
    }
    pg_shader_buffer_model(&core->shader_2d, &core->quad_2d_ctr);
    /*  Our basic 2d quad for drawing images (not centered)  */
    pg_model_init(&core->quad_2d);
    pg_model_quad(&core->quad_2d, (vec2){ 1, 1 });
    mat4_translate(transform, 0.5, 0.5, 0);
    mat4_scale_aniso(transform, transform, 1, -1, 0);
    pg_model_transform(&core->quad_2d, transform);
    pg_model_reserve_component(&core->quad_2d,
        PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    ARR_FOREACH_PTR(core->quad_2d.height, f, i) {
        *f = 1.0f;
    }
    ARR_FOREACH_PTR(core->quad_2d.color, color, i) {
        vec4_set(color->v, 1, 1, 1, 1);
    }
    pg_shader_buffer_model(&core->shader_2d, &core->quad_2d);
    /*  Generate audio  */
    struct pg_audio_envelope env = {
        .attack_time = 0.02, .max = 0.65,
        .decay_time = 0.02, .sustain = 0.3,
        .release_time = 0.1 };
    struct pg_wave menu_wave[4] = {
        PG_WAVE_FUNC_SINE(.frequency = { 440.0f }, .scale = 0.5 ),
        PG_WAVE_FUNC_PERLIN(.frequency = { 1000.0f }),
    };
    pg_audio_alloc(&core->menu_sound, 0.15);
    pg_audio_generate(&core->menu_sound, 0.15, &PG_WAVE_ARRAY(menu_wave, 4), &env);
}

static void list_save(tfFILE* f, void* udata)
{
    struct bork_game_core* core = udata;
    struct bork_save save;
    strncpy(save.name, f->name, 32);
    ARR_PUSH(core->save_files, save);
}

void bork_read_saves(struct bork_game_core* core)
{
    char filename[1024];
    snprintf(filename, 1024, "%ssaves", core->base_path);
    tfTraverse(filename, list_save, core);
}

void bork_delete_save(struct bork_game_core* core, int save_idx)
{
    char filename[1024];
    snprintf(filename, 1024, "%ssaves/%s", core->base_path, core->save_files.data[save_idx].name);
    remove(filename);
    ARR_SPLICE(core->save_files, save_idx, 1);
    printf("%s\n", filename);
}

void bork_draw_fps(struct bork_game_core* core)
{
    struct pg_shader_text fps_text = { .use_blocks = 1 };
    struct pg_shader* shader = &core->shader_text;
    pg_shader_text_resolution(shader, core->screen_size);
    vec4_set(fps_text.block_style[0], 0, 0, 1, 1.2);
    vec4_set(fps_text.block_color[0], 1, 1, 1, 1);
    snprintf(fps_text.block[0], 10, "FPS: %d", (int)pg_framerate());
    pg_shader_text_transform(&core->shader_text, (vec2){ 8, 8 }, (vec2){ 0, 0 });
    pg_shader_text_write(&core->shader_text, &fps_text);
}

void bork_draw_backdrop(struct bork_game_core* core, vec4 color_mod, float t)
{
    static vec4 colors[3] = {
        { 0.8, 0, 0, 0.7 },
        { 0.4, 0.4, 0.4, 0.3 },
        { 0.8, 0, 0, 0.3 },
    };
    static float f[3] = { 0.01, -0.05, 0.1 };
    static float off[3] = { 2, 0.3, 0.5 };
    struct pg_shader* shader = &core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ core->aspect_ratio, 1 });
    pg_shader_2d_texture(shader, &core->backdrop_tex);
    pg_shader_2d_tex_weight(shader, 1);
    pg_shader_2d_set_light(shader, (vec2){ 0, -1 }, (vec3){}, (vec3){ 1, 1, 1 });
    pg_shader_2d_transform(shader, (vec2){}, (vec2){ core->aspect_ratio, 1 }, 0);
    pg_model_begin(&core->quad_2d_ctr, shader);
    int i;
    for(i = 0; i < 3; ++i) {
        vec4 c;
        vec4_mul(c, colors[i], color_mod);
        pg_shader_2d_color_mod(shader, c, (vec4){});
        pg_shader_2d_tex_transform(shader, (vec2){ core->aspect_ratio, 1 },
           (vec2){ cos(t * f[i]) * off[i], sin(t * f[i]) * off[i] });
        pg_model_draw(&core->quad_2d_ctr, NULL);
    }
}

void bork_draw_linear_vignette(struct bork_game_core* core, vec4 color_mod)
{
    struct pg_shader* shader = &core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_ndc(shader, (vec2){ core->aspect_ratio, 1 });
    pg_shader_2d_transform(shader, (vec2){}, (vec2){ core->aspect_ratio, 1 }, 0);
    pg_shader_2d_texture(shader, &core->menu_vignette);
    pg_shader_2d_color_mod(shader, color_mod, (vec4){});
    pg_shader_2d_set_light(shader, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&core->quad_2d_ctr, shader);
    pg_model_draw(&core->quad_2d_ctr, NULL);
}

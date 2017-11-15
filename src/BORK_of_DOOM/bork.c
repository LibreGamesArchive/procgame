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

static const uint8_t control_defaults[BORK_CTRL_COUNT] = {
    [BORK_CTRL_UP] =            SDL_SCANCODE_W,
    [BORK_CTRL_DOWN] =          SDL_SCANCODE_S,
    [BORK_CTRL_LEFT] =          SDL_SCANCODE_A,
    [BORK_CTRL_RIGHT] =         SDL_SCANCODE_D,
    [BORK_CTRL_JUMP] =          SDL_SCANCODE_SPACE,
    [BORK_CTRL_CROUCH] =        SDL_SCANCODE_LCTRL,
    [BORK_CTRL_FLASHLIGHT] =    SDL_SCANCODE_F,
    [BORK_CTRL_FIRE] =          PG_LEFT_MOUSE,
    [BORK_CTRL_RELOAD] =        SDL_SCANCODE_R,
    [BORK_CTRL_DROP] =          SDL_SCANCODE_G,
    [BORK_CTRL_INTERACT] =      SDL_SCANCODE_E,
    [BORK_CTRL_USE_TECH] =      PG_RIGHT_MOUSE,
    [BORK_CTRL_NEXT_TECH] =     SDL_SCANCODE_C,
    [BORK_CTRL_PREV_TECH] =     SDL_SCANCODE_V,
    [BORK_CTRL_NEXT_ITEM] =     PG_MOUSEWHEEL_DOWN,
    [BORK_CTRL_PREV_ITEM] =     PG_MOUSEWHEEL_UP,
    [BORK_CTRL_BIND1] =         SDL_SCANCODE_1,
    [BORK_CTRL_BIND2] =         SDL_SCANCODE_2,
    [BORK_CTRL_BIND3] =         SDL_SCANCODE_3,
    [BORK_CTRL_BIND4] =         SDL_SCANCODE_4,
    [BORK_CTRL_MENU] =          SDL_SCANCODE_ESCAPE,
    [BORK_CTRL_MENU_BACK] =     SDL_SCANCODE_ESCAPE,
    [BORK_CTRL_SELECT] =        SDL_SCANCODE_SPACE,
};


void bork_read_saves(struct bork_game_core* core);

void bork_init(struct bork_game_core* core, char* base_path)
{
    srand(time(NULL));
    int sw, sh;
    pg_screen_size(&sw, &sh);
    *core = (struct bork_game_core) {
        .screen_size = { sw, sh },
        .aspect_ratio = (float)sw / (float)sh };
    if(base_path) {
        core->base_path = base_path;
        core->base_path_len = strlen(core->base_path);
        core->free_base_path = 1;
    } else {
        core->base_path = "./";
        core->base_path_len = 2;
        core->free_base_path = 0;
    }
    bork_load_options(core);
    /*  Set up the gbuffer for deferred shading */
    pg_gbuffer_init(&core->gbuf, sw, sh);
    pg_gbuffer_bind(&core->gbuf, 22, 23, 24, 25);
    pg_viewer_init(&core->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        core->screen_size, (vec2){ 0.01, 256 });
    /*  Set up the shaders  */
    pg_shader_3d(&core->shader_3d);
    pg_shader_2d(&core->shader_2d);
    pg_shader_sprite(&core->shader_sprite);
    pg_shader_text(&core->shader_text);
    pg_ppbuffer_init(&core->ppbuf, sw, sh, 27, 28);
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

void bork_deinit(struct bork_game_core* core)
{
    pg_texture_deinit(&core->env_atlas);
    pg_texture_deinit(&core->editor_atlas);
    pg_texture_deinit(&core->bullet_tex);
    pg_texture_deinit(&core->particle_tex);
    pg_texture_deinit(&core->upgrades_tex);
    pg_texture_deinit(&core->item_tex);
    pg_texture_deinit(&core->enemies_tex);
    pg_texture_deinit(&core->starfield_tex);
    pg_texture_deinit(&core->moon_tex);
    pg_texture_deinit(&core->font);
    pg_texture_deinit(&core->backdrop_tex);
    pg_texture_deinit(&core->menu_vignette);
    pg_texture_deinit(&core->radial_vignette);
    pg_audio_free(&core->menu_sound);
    int i;
    for(i = 0; i < BORK_NUM_SOUNDS; ++i) {
        if(core->sounds[i].len) pg_audio_free(&core->sounds[i]);
    }
    pg_model_deinit(&core->quad_2d);
    pg_model_deinit(&core->quad_2d_ctr);
    pg_model_deinit(&core->bullet_model);
    pg_model_deinit(&core->enemy_model);
    pg_model_deinit(&core->gun_model);
    pg_ppbuffer_deinit(&core->ppbuf);
    pg_gbuffer_deinit(&core->gbuf);
    pg_shader_deinit(&core->shader_3d);
    pg_shader_deinit(&core->shader_2d);
    pg_shader_deinit(&core->shader_sprite);
    pg_shader_deinit(&core->shader_text);
    pg_postproc_deinit(&core->post_blur);
    pg_postproc_deinit(&core->post_screen);
}

void bork_reinit_gfx(struct bork_game_core* core, int sw, int sh, int fullscreen)
{
    core->fullscreen = fullscreen;
    core->screen_size[0] = sw;
    core->screen_size[1] = sh;
    core->aspect_ratio = (float)sw / (float)sh;
    pg_gbuffer_deinit(&core->gbuf);
    pg_ppbuffer_deinit(&core->ppbuf);
    pg_gbuffer_init(&core->gbuf, sw, sh);
    pg_gbuffer_bind(&core->gbuf, 22, 23, 24, 25);
    pg_viewer_init(&core->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        core->screen_size, (vec2){ 0.01, 256 });
    pg_ppbuffer_init(&core->ppbuf, sw, sh, 27, 28);
    pg_window_resize(sw, sh, fullscreen);
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

static void load_wav_from_base_dir(struct bork_game_core* core,
                                   struct pg_audio_chunk* audio,
                                   const char* filename)
{
    char f[1024];
    snprintf(f, 1024, "%s%s", core->base_path, filename);
    pg_audio_load_wav(audio, f);
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
    load_from_base_dir(core, &core->starfield_tex, "res/starfield.png", "res/starfield_lightmap.png");
    pg_texture_bind(&core->starfield_tex, 17, 18);
    /*  Generate the backdrop texture (cloudy reddish fog)  */
    pg_texture_init(&core->backdrop_tex, 256, 256);
    pg_texture_init(&core->menu_vignette, 256, 256);
    pg_texture_init(&core->radial_vignette, 256, 256);
    pg_texture_bind(&core->backdrop_tex, 19, -1);
    pg_texture_bind(&core->menu_vignette, 20, -1);
    pg_texture_bind(&core->radial_vignette, 21, -1);
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
    /*  Load audio  */
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_PICKUP], "res/audio/Pickup.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_PISTOL], "res/audio/Pistol.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_MACHINEGUN], "res/audio/Machine_gun.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_SHOTGUN], "res/audio/Shotgun.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_PLAZGUN], "res/audio/Plaz-gun.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_BULLET_HIT], "res/audio/Bullet_hit.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_PLAZMA_HIT], "res/audio/Plazgun_hit.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_RECYCLER], "res/audio/Recycler.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_DEFENSE_FIELD], "res/audio/Defense_field.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_DOOR_OPEN], "res/audio/Door_open.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_DOOR_CLOSE], "res/audio/Door_close.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_TELEPORT], "res/audio/Teleporter.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_ITEM_LAND], "res/audio/Item_land.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_PLAYER_LAND], "res/audio/Player_land.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_JUMP], "res/audio/Jump.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_FOOTSTEP1], "res/audio/Footstep1.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_FOOTSTEP2], "res/audio/Footstep2.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_FIRE], "res/audio/Fire.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_EXPLOSION], "res/audio/Explosion1.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_HURT], "res/audio/Hurt.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_HUM], "res/audio/Ambient_hum_loop.wav");
    load_wav_from_base_dir(core, &core->sounds[BORK_SND_HISS], "res/audio/Ambient_hiss.wav");
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

void bork_reset_keymap(struct bork_game_core* core)
{
    memcpy(core->ctrl_map, control_defaults, BORK_CTRL_COUNT);
}

void bork_load_options(struct bork_game_core* core)
{
    char filename[1024];
    snprintf(filename, 1024, "%sbork_keymap", core->base_path);
    FILE* f = fopen(filename, "rb");
    if(!f) {
        printf("Could not find keymap file, using defaults.\n");
        memcpy(core->ctrl_map, control_defaults, BORK_CTRL_COUNT);
        return;
    }
    fread(&core->ctrl_map, sizeof(core->ctrl_map), 1, f);
    fclose(f);
}

void bork_save_options(struct bork_game_core* core)
{
    char filename[1024];
    snprintf(filename, 1024, "%sbork_keymap", core->base_path);
    FILE* f = fopen(filename, "wb");
    fwrite(&core->ctrl_map, sizeof(core->ctrl_map), 1, f);
    fclose(f);
    snprintf(filename, 1024, "%soptions.txt", core->base_path);
    f = fopen(filename, "wb");
    fprintf(f, "x:%d\ny:%d\nfullscreen:%d\nmouse:%f",
        (int)core->screen_size[0], (int)core->screen_size[1],
        core->fullscreen, core->mouse_sensitivity);
    fclose(f);

}

static const char* bork_ctrl_names[] = {
    [BORK_CTRL_UP] =            "WALK FORWARD",
    [BORK_CTRL_DOWN] =          "WALK BACKWARD",
    [BORK_CTRL_LEFT] =          "WALK LEFT",
    [BORK_CTRL_RIGHT] =         "WALK RIGHT",
    [BORK_CTRL_JUMP] =          "JUMP",
    [BORK_CTRL_CROUCH] =        "CROUCH",
    [BORK_CTRL_FLASHLIGHT] =    "FLASHLIGHT",
    [BORK_CTRL_FIRE] =          "USE HELD ITEM (SHOOT)",
    [BORK_CTRL_RELOAD] =        "RELOAD",
    [BORK_CTRL_DROP] =          "DROP ITEM",
    [BORK_CTRL_INTERACT] =      "INTERACT",
    [BORK_CTRL_USE_TECH] =      "USE TECH UPGRADE",
    [BORK_CTRL_NEXT_TECH] =     "NEXT UPGRADE",
    [BORK_CTRL_PREV_TECH] =     "PREVIOUS UPGRADE",
    [BORK_CTRL_NEXT_ITEM] =     "NEXT ITEM",
    [BORK_CTRL_PREV_ITEM] =     "PREV ITEM",
    [BORK_CTRL_BIND1] =         "QUICK FETCH 1",
    [BORK_CTRL_BIND2] =         "QUICK FETCH 2",
    [BORK_CTRL_BIND3] =         "QUICK FETCH 3",
    [BORK_CTRL_BIND4] =         "QUICK FETCH 4",
    [BORK_CTRL_MENU] =          "MENU",
    [BORK_CTRL_MENU_BACK] =     "MENU BACK",
    [BORK_CTRL_SELECT] =        "SELECT OPTION",
    [BORK_CTRL_COUNT] =         "NULL"
};

const char* bork_get_ctrl_name(enum bork_control ctrl)
{
    return bork_ctrl_names[ctrl];
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

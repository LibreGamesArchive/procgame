#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

void bork_init(struct bork_game_core* core)
{
    srand(time(NULL));
    core->user_exit = 0;
    /*  Set up the gbuffer for deferred shading */
    int sw, sh;
    pg_screen_size(&sw, &sh);
    vec2_set(core->screen_size, sw, sh);
    core->aspect_ratio = core->screen_size[0] / core->screen_size[1];
    pg_gbuffer_init(&core->gbuf, sw, sh);
    pg_gbuffer_bind(&core->gbuf, 20, 21, 22, 23);
    pg_viewer_init(&core->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        core->screen_size, (vec2){ 0.01, 200 });
    /*  Set up the shaders  */
    pg_shader_3d(&core->shader_3d);
    pg_shader_2d(&core->shader_2d);
    pg_shader_sprite(&core->shader_sprite);
    pg_shader_text(&core->shader_text);
    pg_ppbuffer_init(&core->ppbuf, sw, sh, 24, 25);
    pg_postproc_blur(&core->post_blur, PG_BLUR7);
    /*  Get the models, textures, sounds, etc.*    */
    bork_load_assets(core);
    /*  Attach the font texture to the text shader  */
    pg_shader_text_font(&core->shader_text, &core->font);
    pg_shader_3d_texture(&core->shader_3d, &core->env_atlas);
    pg_shader_2d_texture(&core->shader_2d, &core->editor_atlas);
    pg_shader_sprite_texture(&core->shader_sprite, &core->bullet_tex);
    pg_shader_sprite_tex_frame(&core->shader_sprite, 0);
    /*  Set up the controls */
    int i;
    for(i = 0; i < BORK_CTRL_NULL; ++i) {
        core->ctrl_state[i] = 0;
    }
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

void bork_load_assets(struct bork_game_core* core)
{
    /*  Loading the textures and setting the atlas dimensions   */
    pg_texture_init_from_file(&core->font, "font_8x8.png", NULL);
    pg_texture_set_atlas(&core->font, 8, 8);
    pg_texture_bind(&core->font, 3, 4);
    pg_texture_init_from_file(&core->env_atlas, "res/env_atlas.png", "res/env_atlas_lightmap.png");
    pg_texture_set_atlas(&core->env_atlas, 32, 32);
    pg_texture_bind(&core->env_atlas, 5, 6);
    pg_texture_init_from_file(&core->editor_atlas, "res/editor_atlas.png", NULL);
    pg_texture_set_atlas(&core->editor_atlas, 32, 32);
    pg_texture_bind(&core->editor_atlas, 7, 8);
    pg_texture_init_from_file(&core->bullet_tex, "res/bullets.png", "res/bullets_lightmap.png");
    pg_texture_set_atlas(&core->bullet_tex, 16, 16);
    pg_texture_bind(&core->bullet_tex, 9, 10);
    pg_texture_init_from_file(&core->item_tex, "res/items.png", "res/items_lightmap.png");
    pg_texture_set_atlas(&core->item_tex, 16, 16);
    pg_texture_bind(&core->item_tex, 11, 12);
    /*  Generate the backdrop texture (cloudy reddish fog)  */
    pg_texture_init(&core->backdrop_tex, 256, 256);
    pg_texture_init(&core->menu_vignette, 256, 256);
    pg_texture_bind(&core->backdrop_tex, 13, -1);
    pg_texture_bind(&core->menu_vignette, 14, -1);
    float seed = (float)rand() / RAND_MAX * 1000;
    struct pg_wave backdrop_wave[8] = {
        PG_WAVE_MOD_SEAMLESS_2D(.scale = 0.5),
        PG_WAVE_MOD_OCTAVES(.octaves = 5, .ratio = 3, .decay = 0.5),
        PG_WAVE_FUNC_PERLIN(.frequency = { 4, 4, 4, 4 }, .phase = { seed }),
    };
    struct pg_wave vignette_wave[8] = {
        PG_WAVE_FUNC_MAX(.frequency = {0.5, 1.4}, .scale = 10, .add = -9),
    };
    pg_texture_wave_to_colors(&core->backdrop_tex, &PG_WAVE_ARRAY(backdrop_wave, 8),
                              backdrop_color_func);
    pg_texture_wave_to_colors(&core->menu_vignette, &PG_WAVE_ARRAY(vignette_wave, 8),
                              vignette_color_func);
    pg_texture_buffer(&core->backdrop_tex);
    pg_texture_buffer(&core->menu_vignette);
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
    /*  Our basic 2d quad for drawing images   */
    pg_model_init(&core->quad_2d);
    pg_model_quad(&core->quad_2d, (vec2){ 1, 1 });
    mat4_identity(transform);
    mat4_scale_aniso(transform, transform, 2, -2, 0);
    pg_model_transform(&core->quad_2d, transform);
    pg_model_reserve_component(&core->quad_2d,
        PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    vec4_t* color;
    float* f;
    int i;
    ARR_FOREACH_PTR(core->quad_2d.height, f, i) {
        *f = 1.0f;
    }
    ARR_FOREACH_PTR(core->quad_2d.color, color, i) {
        vec4_set(color->v, 1, 1, 1, 1);
    }
    pg_shader_buffer_model(&core->shader_2d, &core->quad_2d);
}

void bork_poll_input(struct bork_game_core* core)
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) core->user_exit = 1;
        else if(e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            if(core->ctrl_state[BORK_CTRL_FIRE] == 0) {
                core->ctrl_state[BORK_CTRL_FIRE] = BORK_CONTROL_HIT;
            }
        } else if(e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            core->ctrl_state[BORK_CTRL_FIRE] = BORK_CONTROL_RELEASED;
        } else if(e.type == SDL_KEYDOWN) {
            enum bork_control ctrl = BORK_CTRL_NULL;
            switch(e.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE: ctrl = BORK_CTRL_ESCAPE; break;
            case SDL_SCANCODE_W: ctrl = BORK_CTRL_UP; break;
            case SDL_SCANCODE_S: ctrl = BORK_CTRL_DOWN; break;
            case SDL_SCANCODE_A: ctrl = BORK_CTRL_LEFT; break;
            case SDL_SCANCODE_D: ctrl = BORK_CTRL_RIGHT; break;
            case SDL_SCANCODE_E: ctrl = BORK_CTRL_SELECT; break;
            case SDL_SCANCODE_SPACE: ctrl = BORK_CTRL_JUMP; break;
            case SDL_SCANCODE_1: ctrl = BORK_CTRL_BIND1; break;
            case SDL_SCANCODE_2: ctrl = BORK_CTRL_BIND2; break;
            case SDL_SCANCODE_3: ctrl = BORK_CTRL_BIND3; break;
            case SDL_SCANCODE_4: ctrl = BORK_CTRL_BIND4; break;
            default: break;
            }
            if(ctrl != BORK_CTRL_NULL && core->ctrl_state[ctrl] == 0) {
                core->ctrl_state[ctrl] = BORK_CONTROL_HIT;
            }
        } else if(e.type == SDL_KEYUP) {
            enum bork_control ctrl = BORK_CTRL_NULL;
            switch(e.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE: ctrl = BORK_CTRL_ESCAPE; break;
            case SDL_SCANCODE_W: ctrl = BORK_CTRL_UP; break;
            case SDL_SCANCODE_S: ctrl = BORK_CTRL_DOWN; break;
            case SDL_SCANCODE_A: ctrl = BORK_CTRL_LEFT; break;
            case SDL_SCANCODE_D: ctrl = BORK_CTRL_RIGHT; break;
            case SDL_SCANCODE_E: ctrl = BORK_CTRL_SELECT; break;
            case SDL_SCANCODE_SPACE: ctrl = BORK_CTRL_JUMP; break;
            case SDL_SCANCODE_1: ctrl = BORK_CTRL_BIND1; break;
            case SDL_SCANCODE_2: ctrl = BORK_CTRL_BIND2; break;
            case SDL_SCANCODE_3: ctrl = BORK_CTRL_BIND3; break;
            case SDL_SCANCODE_4: ctrl = BORK_CTRL_BIND4; break;
            default: break;
            }
            if(ctrl != BORK_CTRL_NULL) {
                core->ctrl_state[ctrl] = BORK_CONTROL_RELEASED;
            }
        }
    }
}

void bork_update_inputs(struct bork_game_core* core)
{
    int i;
    for(i = 0; i < BORK_CTRL_NULL; ++i) {
        switch(core->ctrl_state[i]) {
        case BORK_CONTROL_HIT:
            core->ctrl_state[i] = BORK_CONTROL_HELD;
            break;
        case BORK_CONTROL_RELEASED:
            core->ctrl_state[i] = 0;
            break;
        }
    }
}

void bork_draw_fps(struct bork_game_core* core)
{
    struct pg_shader_text fps_text = { .use_blocks = 1 };
    struct pg_shader* shader = &core->shader_text;
    pg_shader_text_resolution(shader, core->screen_size);
    vec4_set(fps_text.block_style[0], 0, 0, 1, 1.2);
    vec4_set(fps_text.block_color[0], 1, 1, 1, 1);
    snprintf(fps_text.block[0], 10, "FPS: %d", (int)pg_framerate());
    pg_shader_text_transform(&core->shader_text, (vec2){ 0, 0 }, (vec2){ 8, 8 });
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
    pg_shader_2d_resolution(shader, (vec2){ core->aspect_ratio, 1 });
    pg_shader_2d_texture(shader, &core->backdrop_tex);
    pg_shader_2d_tex_weight(shader, 1);
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_transform(shader, (vec2){}, (vec2){ core->aspect_ratio, 1 }, 0);
    pg_model_begin(&core->quad_2d, shader);
    int i;
    for(i = 0; i < 3; ++i) {
        vec4 c;
        vec4_mul(c, colors[i], color_mod);
        pg_shader_2d_color_mod(shader, c);
        pg_shader_2d_tex_transform(shader, (vec2){ core->aspect_ratio, 1 },
           (vec2){ cos(t * f[i]) * off[i], sin(t * f[i]) * off[i] });
        pg_model_draw(&core->quad_2d, NULL);
    }
}

void bork_draw_linear_vignette(struct bork_game_core* core, vec4 color_mod)
{
    struct pg_shader* shader = &core->shader_2d;
    pg_shader_2d_ndc(shader, (vec2){ core->aspect_ratio, 1 });
    pg_shader_2d_transform(shader, (vec2){}, (vec2){ core->aspect_ratio, 1 }, 0);
    pg_shader_2d_texture(shader, &core->menu_vignette);
    pg_shader_2d_color_mod(shader, color_mod);
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_model_begin(&core->quad_2d, shader);
    pg_model_draw(&core->quad_2d, NULL);
}

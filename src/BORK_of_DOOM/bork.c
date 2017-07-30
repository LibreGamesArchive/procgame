#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/wave_defs.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

void bork_init(struct bork_game_core* core)
{
    /*  Set up the gbuffer for deferred shading */
    pg_screen_size(&core->screen_size[0], &core->screen_size[1]);
    pg_gbuffer_init(&core->gbuf, core->screen_size[0], core->screen_size[0]);
    pg_gbuffer_bind(&core->gbuf, 20, 21, 22, 23);
    pg_viewer_init(&core->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        (vec2){ core->screen_size[0], core->screen_size[1] },
        (vec2){ 0.01, 200 });
    /*  Set up the shaders  */
    pg_shader_3d(&core->shader_3d);
    pg_shader_2d(&core->shader_2d);
    pg_shader_sprite(&core->shader_sprite);
    pg_shader_text(&core->shader_text);
    /*  Get the models, textures, sounds, etc.*    */
    bork_load_assets(core);
    /*  Attach the font texture to the text shader  */
    pg_shader_text_set_font(&core->shader_text, &core->font);
    pg_shader_3d_set_texture(&core->shader_3d, &core->env_atlas);
    pg_shader_2d_set_texture(&core->shader_2d, &core->editor_atlas);
    pg_shader_sprite_set_texture(&core->shader_sprite, &core->bullet_tex);
    pg_shader_sprite_set_tex_frame(&core->shader_sprite, 0);
}

void bork_load_assets(struct bork_game_core* core)
{
    /*  Loading the textures and setting the atlas dimensions   */
    pg_texture_init_from_file(&core->font, "font_5x8.png", NULL);
    pg_texture_set_atlas(&core->font, 5, 8);
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
    /*  Generate the basic models, just quads for now   */
    mat4 transform;
    mat4_identity(transform);
    mat4_rotate_X(transform, transform, M_PI * 1.5);
    mat4_scale(transform, transform, 2);
    pg_model_init(&core->enemy_model);
    pg_model_quad(&core->enemy_model, (vec2){ 1, 1 });
    pg_model_transform(&core->enemy_model, transform);
    pg_shader_buffer_model(&core->shader_sprite, &core->enemy_model);
    mat4_scale(transform, transform, 0.25);
    pg_model_init(&core->bullet_model);
    pg_model_quad(&core->bullet_model, (vec2){ 1, 1 });
    pg_model_transform(&core->bullet_model, transform);
    pg_model_precalc_ntb(&core->bullet_model);
    pg_shader_buffer_model(&core->shader_sprite, &core->bullet_model);
    pg_shader_buffer_model(&core->shader_3d, &core->bullet_model);
}


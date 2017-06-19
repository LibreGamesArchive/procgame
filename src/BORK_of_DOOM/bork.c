#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/wave_defs.h"
#include "bork.h"
#include "map_area.h"

void bork_init(struct bork_game_core* core)
{
    /*  Set up the gbuffer for deferred shading */
    pg_screen_size(&core->screen_size[0], &core->screen_size[1]);
    pg_gbuffer_init(&core->gbuf, core->screen_size[0], core->screen_size[0]);
    pg_gbuffer_bind(&core->gbuf, 20, 21, 22, 23);
    pg_viewer_init(&core->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
        (vec2){ core->screen_size[0], core->screen_size[1] },
        (vec2){ 0.1, 200 });
    /*  Set up the shaders  */
    pg_shader_3d(&core->shader_3d);
    pg_shader_text(&core->shader_text);
    /*  Get the models, textures, sounds, etc.*    */
    bork_load_assets(core);
    /*  Attach the font texture to the text shader  */
    pg_shader_text_set_font(&core->shader_text, &core->font);
    pg_shader_3d_set_texture(&core->shader_3d, &core->env_atlas);
}

void bork_load_assets(struct bork_game_core* core)
{
    /*  Loading the font texture    */
    pg_texture_init_from_file(&core->font, "font_5x8.png", NULL);
    pg_texture_set_atlas(&core->font, 5, 8);
    pg_texture_bind(&core->font, 3, 4);
    pg_texture_init_from_file(&core->env_atlas, "res/env_atlas.png", "res/env_atlas_lightmap.png");
    pg_texture_set_atlas(&core->env_atlas, 32, 32);
    pg_texture_bind(&core->env_atlas, 5, 6);
    pg_model_init(&core->map_model);
}


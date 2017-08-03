#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

struct bork_editor_data {
    struct bork_game_core* core;
    const uint8_t* kb_state;
    struct pg_model tile_quad;
    mat4 pixel_transform;
    struct bork_map map;
    enum bork_area current_area;
    struct bork_tile current_tile;
    int select_mode;
    int selected_level;
    int selection[4];
    int cursor[2];
};

static void bork_editor_update(struct pg_game_state* state);
static void bork_editor_draw(struct pg_game_state* state);

void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 0, 0);
    struct bork_editor_data* d = malloc(sizeof(*d));
    *d = (struct bork_editor_data) {
        .core = core,
        .kb_state = SDL_GetKeyboardState(NULL) };
    //SDL_SetRelativeMouseMode(SDL_TRUE);
    /*  Generate the basic quad model   */
    pg_model_init(&d->tile_quad);
    pg_model_quad(&d->tile_quad, (vec2){ 1, 1 });
    pg_model_reserve_component(&d->tile_quad,
        PG_MODEL_COMPONENT_COLOR | PG_MODEL_COMPONENT_HEIGHT);
    vec4_t* color;
    float* f;
    int i;
    ARR_FOREACH_PTR(d->tile_quad.height, f, i) {
        *f = 1.0f;
    }
    ARR_FOREACH_PTR(d->tile_quad.color, color, i) {
        vec4_set(color->v, 1, 1, 1, 1);
    }
    pg_shader_buffer_model(&core->shader_2d, &d->tile_quad);
    /*  Init the map    */
    bork_map_init(&d->map, core);
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = bork_editor_update;
    state->draw = bork_editor_draw;
    state->deinit = free;
}

static void bork_editor_update(struct pg_game_state* state)
{
    struct bork_editor_data* d = state->data;
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->running = 0;
        else if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.scancode) {
            case SDL_SCANCODE_H:
                if(e.key.keysym.mod & KMOD_LCTRL) {
                    if(e.key.keysym.mod & KMOD_LSHIFT) {
                        d->current_tile.orientation |= 1 << PG_RIGHT;
                    } else d->current_tile.orientation = 1 << PG_RIGHT;
                } else d->cursor[0] = MOD(d->cursor[0] - 1, 32);
                break;
            case SDL_SCANCODE_L:
                if(e.key.keysym.mod & KMOD_LCTRL) {
                    if(e.key.keysym.mod & KMOD_LSHIFT) {
                        d->current_tile.orientation |= 1 << PG_LEFT;
                    } else d->current_tile.orientation = 1 << PG_LEFT;
                } else d->cursor[0] = MOD(d->cursor[0] + 1, 32);
                break;
            case SDL_SCANCODE_J:
                if(e.key.keysym.mod & KMOD_LCTRL) {
                    if(e.key.keysym.mod & KMOD_LSHIFT) {
                        d->current_tile.orientation |= 1 << PG_FRONT;
                    } else d->current_tile.orientation = 1 << PG_FRONT;
                } else d->cursor[1] = MOD(d->cursor[1] + 1, 32);
                break;
            case SDL_SCANCODE_K:
                if(e.key.keysym.mod & KMOD_LCTRL) {
                    if(e.key.keysym.mod & KMOD_LSHIFT) {
                        d->current_tile.orientation |= 1 << PG_BACK;
                    } else d->current_tile.orientation = 1 << PG_BACK;
                } else d->cursor[1] = MOD(d->cursor[1] - 1, 32);
                break;
            case SDL_SCANCODE_V:
                if(!d->select_mode) {
                    d->select_mode = 1;
                    d->selection[0] = d->cursor[0];
                    d->selection[1] = d->cursor[1];
                    d->selection[2] = d->cursor[0];
                    d->selection[3] = d->cursor[1];
                } else d->select_mode = 0;
                break;
            case SDL_SCANCODE_U:
                if(e.key.keysym.mod & KMOD_LCTRL)
                    d->selected_level = MOD(d->selected_level + 1, 32);
                else d->current_tile.type = MOD(d->current_tile.type + 1, BORK_TILE_COUNT);
                break;
            case SDL_SCANCODE_D:
                if(e.key.keysym.mod & KMOD_LCTRL)
                    d->selected_level = MOD(d->selected_level - 1, 32);
                else {
                    if(!d->current_tile.type) d->current_tile.type = BORK_TILE_COUNT;
                    else --d->current_tile.type;
                }
                break;
            case SDL_SCANCODE_W:
                if(!(e.key.keysym.mod & KMOD_LCTRL)) break;
                bork_map_write_to_file(&d->map, "test.bork_map");
                break;
            case SDL_SCANCODE_SPACE:
                if(d->selection[0] == -1) {
                    struct bork_tile* tile = bork_map_tile_ptr(&d->map, d->current_area,
                        d->cursor[0], d->cursor[1], d->selected_level);
                    *tile = d->current_tile;
                } else {
                    int select_rect[4] = {
                        MIN(d->selection[0], d->selection[2]), MIN(d->selection[1], d->selection[3]),
                        MAX(d->selection[0], d->selection[2]), MAX(d->selection[1], d->selection[3]) };
                    ++select_rect[2];
                    ++select_rect[3];
                    int x, y;
                    for(x = select_rect[0]; x < select_rect[2]; ++x) {
                        for(y = select_rect[1]; y < select_rect[3]; ++y) {
                            struct bork_tile* tile = bork_map_tile_ptr(&d->map, d->current_area,
                                x, y, d->selected_level);
                            *tile = d->current_tile;
                        }
                    }
                }
                break;
            default: break;
            }
        }
    }
    if(d->select_mode) {
        d->selection[2] = d->cursor[0];
        d->selection[3] = d->cursor[1];
    }
}

static void bork_editor_draw(struct pg_game_state* state)
{
    struct bork_editor_data* d = state->data;
    pg_screen_dst();
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_2d_resolution(shader, d->core->screen_size);
    pg_shader_2d_set_texture(shader, &d->core->editor_atlas);
    pg_shader_begin(shader, NULL);
    pg_model_begin(&d->tile_quad, shader);
    pg_shader_2d_set_tex_weight(shader, 1.0f);
    pg_shader_2d_set_tex_frame(shader, 2);
    pg_shader_2d_set_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    int select_rect[4] = {
        MIN(d->selection[0], d->selection[2]), MIN(d->selection[1], d->selection[3]),
        MAX(d->selection[0], d->selection[2]), MAX(d->selection[1], d->selection[3]) };
    int x, y;
    for(x = 0; x < 32; ++x) {
        for(y = 0; y < 32; ++y) {
            if(x == d->cursor[0] && y == d->cursor[1]) continue;
            struct bork_tile* tile = bork_map_tile_ptr(&d->map,
                d->current_area, x, y, d->selected_level);
            pg_shader_2d_set_tex_frame(&d->core->shader_2d, tile->type);
            if(x >= select_rect[0] && x <= select_rect[2]
            && y >= select_rect[1] && y <= select_rect[3]) {
                pg_shader_2d_set_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.5 });
            } else {
                pg_shader_2d_set_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
            }
            pg_shader_2d_transform(&d->core->shader_2d,
                (vec2){x * 16 + 64, y * 16 + 32 },
                (vec2){ 16, 16 }, 0);
            pg_model_draw(&d->tile_quad, NULL);
        }
    }
    /*  Get the stuff for the currently selected tile   */
    struct bork_tile* tile = bork_map_tile_ptr(&d->map, d->current_area,
        d->cursor[0], d->cursor[1], d->selected_level);
    const struct bork_tile_detail* detail = bork_tile_detail(tile->type);
    const struct bork_tile_detail* cursor_detail = bork_tile_detail(d->current_tile.type);
    /*  Draw the scaled-up view of the currently selected tile in the map   */
    pg_shader_2d_set_tex_frame(&d->core->shader_2d, tile->type);
    pg_shader_2d_set_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ d->cursor[0] * 16 + 64, d->cursor[1] * 16 + 32 },
        (vec2){ 32, 32 }, 0);
    pg_model_draw(&d->tile_quad, NULL);
    if(detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(tile->orientation & (1 << i))) continue;
            pg_shader_2d_set_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->tile_quad, NULL);
        }
    }
    /*  Draw the current "brush" tile   */
    pg_shader_2d_set_tex_frame(&d->core->shader_2d, d->current_tile.type);
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 700, 128 }, (vec2){ 32, 32 }, 0);
    pg_model_draw(&d->tile_quad, NULL);
    if(cursor_detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(d->current_tile.orientation & (1 << i))) continue;
            pg_shader_2d_set_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->tile_quad, NULL);
        }
    }
    /*  Text    */
    shader = &d->core->shader_text;
    pg_shader_text_resolution(shader, d->core->screen_size);
    pg_shader_begin(shader, NULL);
    pg_shader_text_transform(shader, (vec2){ 740, 40 }, (vec2){ 10, 16 });
    /*  Right-side UI text  */
    struct pg_shader_text text = { .use_blocks = 2 };
    snprintf(text.block[0], 64, "%s : %d",
             BORK_AREA_STRING[d->current_area], d->selected_level);
    vec4_set(text.block_style[0], 0, 0, 2, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    snprintf(text.block[1], 64, "%s", cursor_detail->name);
    vec4_set(text.block_style[1], 0, 5, 1, 1.2);
    vec4_set(text.block_color[1], 1, 0, 0, 1);
    pg_shader_text_write(shader, &text);
    /*  Write the FPS   */
    bork_draw_fps(d->core);
}



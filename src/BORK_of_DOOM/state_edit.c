#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

struct bork_editor_data {
    struct bork_game_core* core;
    int userexit;
    const uint8_t* kb_state;
    mat4 pixel_transform;
    struct bork_map map;
    enum bork_area current_area;
    struct bork_tile current_tile;
    enum {
        BORK_EDIT_MAP,
        BORK_EDIT_TILE_ITEMS,
        BORK_EDIT_ITEM,
    } edit_mode;

    int item_selection;
    int select_mode;    /*  Whether the selection is being drawn    */
    int selected_level;
    int selection[4];
    int cursor[2];
};

static void bork_editor_update(struct pg_game_state* state);
static void bork_editor_draw(struct pg_game_state* state);

void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 3);
    struct bork_editor_data* d = malloc(sizeof(*d));
    *d = (struct bork_editor_data) {
        .core = core,
        .kb_state = SDL_GetKeyboardState(NULL) };
    //SDL_SetRelativeMouseMode(SDL_TRUE);
    /*  Init the map    */
    bork_map_init(&d->map, core);
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = bork_editor_update;
    state->draw = bork_editor_draw;
    state->deinit = free;
}

static void bork_editor_update_map(struct bork_editor_data* d)
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) d->userexit = 1;
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
            case SDL_SCANCODE_I:
                
            default: break;
            }
        }
    }
    if(d->select_mode) {
        d->selection[2] = d->cursor[0];
        d->selection[3] = d->cursor[1];
    }
}

static void bork_editor_update_tile_items(struct bork_editor_data* d)
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) d->userexit = 1;
        else if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.scancode) {
            case SDL_SCANCODE_H:
                break;
            case SDL_SCANCODE_L:
                break;
            case SDL_SCANCODE_J:
                d->item_selection = 0;
                break;
            case SDL_SCANCODE_K:
                break;
            case SDL_SCANCODE_V:
                break;
            case SDL_SCANCODE_U:
                break;
            case SDL_SCANCODE_D:
                break;
            case SDL_SCANCODE_W:
                break;
            case SDL_SCANCODE_SPACE:
                break;
            case SDL_SCANCODE_I:
                break;
            default: break;
            }
        }
    }
}

static void bork_editor_update(struct pg_game_state* state)
{
    struct bork_editor_data* d = state->data;
    switch(d->edit_mode) {
    case BORK_EDIT_MAP: bork_editor_update_map(d); break;
    case BORK_EDIT_TILE_ITEMS: bork_editor_update_tile_items(d); break;
    //case BORK_EDIT_ITEM: bork_editor_update_item(d);
    }
    if(d->userexit) state->running = 0;
}

static void bork_editor_draw(struct pg_game_state* state)
{
    struct bork_editor_data* d = state->data;
    struct pg_shader* shader = &d->core->shader_2d;
    pg_screen_dst();
    pg_shader_begin(shader, NULL);
    bork_draw_backdrop(d->core, (vec4){ 1, 1, 1, 0.5 },
        (float)state->ticks / (float)state->tick_rate);
    bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 0.8 });
    /*  Draw the map slice  */
    shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_model_begin(&d->core->quad_2d, shader);
    pg_shader_2d_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_2d_texture(shader, &d->core->editor_atlas);
    pg_shader_2d_tex_weight(shader, 1.0f);
    pg_shader_2d_tex_frame(shader, 2);
    pg_shader_2d_color_mod(shader, (vec4){ 1, 1, 1, 1 });
    int select_rect[4] = {
        MIN(d->selection[0], d->selection[2]), MIN(d->selection[1], d->selection[3]),
        MAX(d->selection[0], d->selection[2]), MAX(d->selection[1], d->selection[3]) };
    int x, y;
    for(x = 0; x < 32; ++x) {
        for(y = 0; y < 32; ++y) {
            if(x == d->cursor[0] && y == d->cursor[1]) continue;
            struct bork_tile* tile = bork_map_tile_ptr(&d->map,
                d->current_area, x, y, d->selected_level);
            pg_shader_2d_tex_frame(&d->core->shader_2d, tile->type);
            if(x >= select_rect[0] && x <= select_rect[2]
            && y >= select_rect[1] && y <= select_rect[3]) {
                pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 0.5 });
            } else {
                pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
            }
            pg_shader_2d_transform(&d->core->shader_2d,
                (vec2){ 0.2 + 0.02 * x, 0.2 + 0.02 * y },
                (vec2){ 0.01, 0.01 }, 0);
            pg_model_draw(&d->core->quad_2d, NULL);
        }
    }
    /*  Get the stuff for the currently selected tile   */
    struct bork_tile* tile = bork_map_tile_ptr(&d->map, d->current_area,
        d->cursor[0], d->cursor[1], d->selected_level);
    const struct bork_tile_detail* detail = bork_tile_detail(tile->type);
    const struct bork_tile_detail* cursor_detail = bork_tile_detail(d->current_tile.type);
    /*  Draw the scaled-up view of the currently selected tile in the map   */
    pg_shader_2d_tex_frame(&d->core->shader_2d, d->current_tile.type);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ d->cursor[0] * 0.02 + 0.2, d->cursor[1] * 0.02 + 0.2 },
        (vec2){ 0.02, 0.02 }, 0);
    pg_model_draw(&d->core->quad_2d, NULL);
    if(detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(tile->orientation & (1 << i))) continue;
            pg_shader_2d_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->core->quad_2d, NULL);
        }
    }
    /*  Draw the current "brush" tile   */
    pg_shader_2d_tex_frame(&d->core->shader_2d, tile->type);
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.95, 0.2 }, (vec2){ 0.03, 0.03 }, 0);
    pg_model_draw(&d->core->quad_2d, NULL);
    if(cursor_detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(d->current_tile.orientation & (1 << i))) continue;
            pg_shader_2d_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->core->quad_2d, NULL);
        }
    }
    /*  Text UI */
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    /*  Right-side UI text  */
    struct pg_shader_text text = { .use_blocks = 7 };
    snprintf(text.block[0], 64, "LEVEL: %d", d->selected_level);
    vec4_set(text.block_style[0], 0.25, 0.1, 0.02, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    snprintf(text.block[1], 64, "TILE DETAIL: %s", detail->name);
    vec4_set(text.block_style[1], 1, 0.2, 0.02, 1.2);
    vec4_set(text.block_color[1], 1, 1, 1, 1);
    snprintf(text.block[2], 64, "ITEMS: %d", 0);
    vec4_set(text.block_style[2], 1, 0.23, 0.02, 1.2);
    vec4_set(text.block_color[2], 1, 1, 1, 1);
    snprintf(text.block[3], 64, "[HJKL] MOVE CURSOR        [U/D] CHANGE CURSOR TILE");
    vec4_set(text.block_style[3], 0.25, 0.85, 0.02, 1.2);
    vec4_set(text.block_color[3], 1, 1, 1, 1);
    snprintf(text.block[4], 64, "[V] START/END SELECT      [SPACE] PLACE TILES");
    vec4_set(text.block_style[4], 0.25, 0.88, 0.02, 1.2);
    vec4_set(text.block_color[4], 1, 1, 1, 1);
    snprintf(text.block[5], 64, "[CTRL-HJKL] SET DIRECTION [CTRL-SHIFT-HJKL] ADD DIRECTION");
    vec4_set(text.block_style[5], 0.25, 0.91, 0.02, 1.2);
    vec4_set(text.block_color[5], 1, 1, 1, 1);
    snprintf(text.block[6], 64, "[CTRL-U/D] CHANGE LEVEL   [I] EDIT ITEMS");
    vec4_set(text.block_style[6], 0.25, 0.94, 0.02, 1.2);
    vec4_set(text.block_color[6], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
    bork_draw_fps(d->core);
}


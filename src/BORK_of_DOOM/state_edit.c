#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "game_states.h"

static void bork_editor_write_map(struct bork_editor_map* map, char* filename);
static void bork_editor_update(struct pg_game_state* state);
static void bork_editor_draw(struct pg_game_state* state);

void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 3);
    bork_grab_mouse(core, 0);
    struct bork_editor_data* d = malloc(sizeof(*d));
    *d = (struct bork_editor_data) {
        .core = core,
        .kb_state = SDL_GetKeyboardState(NULL) };
    //SDL_SetRelativeMouseMode(SDL_TRUE);
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = bork_editor_update;
    state->draw = bork_editor_draw;
    state->deinit = free;
}

static void bork_editor_update_map(struct bork_editor_data* d)
{
    uint8_t* ctrl = d->core->ctrl_state;
    if(bork_input_event(d->core, BORK_LEFT_MOUSE, BORK_CONTROL_HIT)) {
        vec2 click = { d->core->mouse_pos[0] / d->core->screen_size[1],
                       d->core->mouse_pos[1] / d->core->screen_size[1] };
        vec2_sub(click, click, (vec2){ 0.19, 0.19 });
        vec2_scale(click, click, (1 / (0.02 * 32)) * 32);
        printf("%f %f\n", click[0], click[1]);
        if(!(click[0] < 0 || click[0] >= 32 || click[1] < 0 || click[1] >= 32)) {
            struct bork_editor_entity new_ent = {
                .type = d->ent_type,
                .pos = { click[0], click[1], d->cursor[2] + 0.5 } };
            ARR_PUSH(d->map.ents, new_ent);
        }
    }
    if(bork_input_event(d->core, SDL_SCANCODE_ESCAPE, BORK_CONTROL_HIT)) {
        d->select_mode = MAX(d->select_mode - 1, -1);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_H, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL]) {
            if(ctrl[SDL_SCANCODE_LSHIFT]) {
                d->current_tile.dir |= 1 << PG_RIGHT;
            } else d->current_tile.dir = 1 << PG_RIGHT;
        } else d->cursor[0] = MOD(d->cursor[0] - 1, 32);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_L, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL]) {
            if(ctrl[SDL_SCANCODE_LSHIFT]) {
                d->current_tile.dir |= 1 << PG_LEFT;
            } else d->current_tile.dir = 1 << PG_LEFT;
        } else d->cursor[0] = MOD(d->cursor[0] + 1, 32);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_J, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL]) {
            if(ctrl[SDL_SCANCODE_LSHIFT]) {
                d->current_tile.dir |= 1 << PG_FRONT;
            } else d->current_tile.dir = 1 << PG_FRONT;
        } else d->cursor[1] = MOD(d->cursor[1] + 1, 32);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_K, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL]) {
            if(ctrl[SDL_SCANCODE_LSHIFT]) {
                d->current_tile.dir |= 1 << PG_BACK;
            } else d->current_tile.dir = 1 << PG_BACK;
        } else d->cursor[1] = MOD(d->cursor[1] - 1, 32);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_V, BORK_CONTROL_HIT)) {
        if(d->select_mode < 1) {
            d->select_mode = 1;
            d->selection[0] = d->cursor[0];
            d->selection[1] = d->cursor[1];
            d->selection[2] = d->cursor[0];
            d->selection[3] = d->cursor[1];
        } else d->select_mode = 0;
    }
    if(bork_input_event(d->core, SDL_SCANCODE_U, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL])
            d->cursor[2] = MOD(d->cursor[2] + 1, 32);
        else if(ctrl[SDL_SCANCODE_LSHIFT])
            d->ent_type = MOD((int)d->ent_type + 1, BORK_ENTITY_TYPES);
        else d->current_tile.type = MOD(d->current_tile.type + 1, BORK_TILE_COUNT);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_Y, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL])
            d->cursor[2] = MOD(d->cursor[2] - 1, 32);
        else if(ctrl[SDL_SCANCODE_LSHIFT])
            d->ent_type = MOD((int)d->ent_type - 1, BORK_ENTITY_TYPES);
        else d->current_tile.type = MOD(d->current_tile.type - 1, BORK_TILE_COUNT);
    }
    if(bork_input_event(d->core, SDL_SCANCODE_R, BORK_CONTROL_HIT)) {
        if(ctrl[SDL_SCANCODE_LCTRL]) bork_editor_write_map(&d->map, "test.bork_map");
        else if(ctrl[SDL_SCANCODE_LSHIFT]) bork_editor_load_map(&d->map, "test.bork_map");
    }
    if(bork_input_event(d->core, SDL_SCANCODE_SPACE, BORK_CONTROL_HIT)) {
        if(d->select_mode == -1) {
            struct bork_editor_tile* tile = 
                &d->map.tiles[d->cursor[0]][d->cursor[1]][d->cursor[2]];
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
                    struct bork_editor_tile* tile = &d->map.tiles[x][y][d->cursor[2]];
                    *tile = d->current_tile;
                }
            }
        }
    }
    if(d->select_mode == 1) {
        d->selection[2] = d->cursor[0];
        d->selection[3] = d->cursor[1];
    }
}

static void bork_editor_update(struct pg_game_state* state)
{
    struct bork_editor_data* d = state->data;
    bork_ack_input(d->core);
    bork_poll_input(d->core);
    if(d->core->user_exit) state->running = 0;
    bork_editor_update_map(d);
}

static void bork_editor_draw_items(struct bork_editor_data* d)
{
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_2d_texture(shader, &d->core->bullet_tex);
    pg_shader_2d_tex_frame(shader, 0);
    struct pg_model* model = &d->core->quad_2d;
    pg_model_begin(model, shader);
    int i;
    struct bork_editor_entity* ent;
    ARR_FOREACH_PTR(d->map.ents, ent, i) {
        if(!(ent->pos[2] >= d->cursor[2] && ent->pos[2] <= d->cursor[2] + 1))
            continue;
        vec2 pos = { ent->pos[0] / 32 * 0.02 * 32 + 0.19,
                     ent->pos[1] / 32 * 0.02 * 32 + 0.19 };
        pg_shader_2d_transform(shader, pos, (vec2){ 0.01, 0.01 }, 0);
        pg_model_draw(model, NULL);
    }
}

static void bork_editor_draw_items_text(struct bork_editor_data* d)
{
    struct pg_shader_text text = { .use_blocks = 1 };
    struct pg_shader* shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    pg_shader_text_write(shader, &text);
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
            struct bork_editor_tile* tile = &d->map.tiles[x][y][d->cursor[2]];
            pg_shader_2d_tex_frame(&d->core->shader_2d, tile->type);
            if(d->select_mode >= 0
            && x >= select_rect[0] && x <= select_rect[2]
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
    struct bork_editor_tile* tile = &d->map.tiles[d->cursor[0]][d->cursor[1]][d->cursor[2]];
    const struct bork_tile_detail* detail = bork_tile_detail(tile->type);
    const struct bork_tile_detail* cursor_detail = bork_tile_detail(d->current_tile.type);
    /*  Draw the scaled-up view of the currently selected tile in the map   */
    pg_shader_2d_tex_frame(&d->core->shader_2d, d->current_tile.type);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ d->cursor[0] * 0.02 + 0.2, d->cursor[1] * 0.02 + 0.2 },
        (vec2){ 0.02, 0.02 }, 0);
    pg_model_draw(&d->core->quad_2d, NULL);
    if(cursor_detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(d->current_tile.dir & (1 << i))) continue;
            pg_shader_2d_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->core->quad_2d, NULL);
        }
    }
    /*  Draw the current "brush" tile   */
    pg_shader_2d_tex_frame(&d->core->shader_2d, tile->type);
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.95, 0.2 }, (vec2){ 0.03, 0.03 }, 0);
    pg_model_draw(&d->core->quad_2d, NULL);
    if(detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(tile->dir & (1 << i))) continue;
            pg_shader_2d_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->core->quad_2d, NULL);
        }
    }
    bork_editor_draw_items(d);
    /*  Text UI */
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    /*  Right-side UI text  */
    struct pg_shader_text text = { .use_blocks = 3 };
    snprintf(text.block[0], 64, "LEVEL: %d", d->cursor[2]);
    vec4_set(text.block_style[0], 0.25, 0.1, 0.02, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    snprintf(text.block[1], 64, "TILE DETAIL: %s", detail->name);
    vec4_set(text.block_style[1], 1, 0.2, 0.02, 1.2);
    vec4_set(text.block_color[1], 1, 1, 1, 1);
    const struct bork_entity_profile* prof = &BORK_ENT_PROFILES[d->ent_type];
    snprintf(text.block[2], 64, "PLACING ENTITY: %s", prof->name);
    vec4_set(text.block_style[2], 1, 0.23, 0.02, 1.2);
    vec4_set(text.block_color[2], 1, 1, 1, 1);
    pg_shader_text_write(shader, &text);
    bork_editor_draw_items_text(d);
    bork_draw_fps(d->core);
}

static void bork_editor_write_map(struct bork_editor_map* map, char* filename)
{
    FILE* file = fopen(filename, "w");
    if(!file) {
        printf("BORK map writing error: could not open file %s\n", filename);
    }
    fwrite(map->tiles, sizeof(struct bork_editor_tile), 32 * 32 *32, file);
    fwrite(&map->ents.len, sizeof(map->ents.len), 1, file);
    fwrite(map->ents.data, sizeof(struct bork_editor_entity), map->ents.len, file);
    fclose(file);
}

int bork_editor_load_map(struct bork_editor_map* map, char* filename)
{
    FILE* file = fopen(filename, "r");
    if(!file) {
        printf("BORK map loading error: could not open file %s\n", filename);
        return 0;
    }
    fread(map->tiles, sizeof(struct bork_editor_tile), 32 * 32 *32, file);
    size_t num_items = 0;
    fread(&num_items, sizeof(num_items), 1, file);
    ARR_RESERVE(map->ents, num_items);
    fread(map->ents.data, sizeof(struct bork_editor_entity), num_items, file);
    map->ents.len = num_items;
    fclose(file);
    return 1;
}

void bork_editor_complete_entity(struct bork_entity* ent,
                                 struct bork_editor_entity* ed)
{
    *ent = (struct bork_entity) {
        .pos = { ed->pos[0] * 2, ed->pos[1] * 2, ed->pos[2] * 2 },
        .flags = BORK_ENT_PROFILES[ed->type].base_flags,
        .type = ed->type,
    };
}

void bork_editor_complete_map(struct bork_map* map, struct bork_editor_map* ed_map)
{
    bork_map_init(map);
    int i, j, k;
    for(i = 0; i < 32; ++i) {
        for(j = 0; j < 32; ++j) {
            for(k = 0; k < 32; ++k) {
                struct bork_tile* tile = &map->data[i][j][k];
                *tile = (struct bork_tile) {
                    .type = ed_map->tiles[i][j][k].type,
                    .orientation = ed_map->tiles[i][j][k].dir };
                if(tile->type == BORK_TILE_EDITOR_DOOR) {
                    struct bork_map_object new_obj = {
                        .type = BORK_MAP_OBJ_DOOR, .x = i, .y = j, .z = k };
                    if(tile->orientation & ((1 << PG_LEFT) | (1 << PG_RIGHT))) new_obj.door.dir = 1;
                    ARR_PUSH(map->objects, new_obj);
                    tile->type = BORK_TILE_ATMO;
                } else if(tile->type == BORK_TILE_EDITOR_LIGHT1) {
                    struct bork_map_object new_obj = {
                        .type = BORK_MAP_OBJ_LIGHT,
                        .light = { 2, 2, 1.8, 8 },
                        .x = i, .y = j, .z = k };
                    ARR_PUSH(map->objects, new_obj);
                    tile->type = BORK_TILE_ATMO;
                }
            }
        }
    }
    bork_entity_t ent_arr = bork_entity_new(ed_map->ents.len);
    struct bork_editor_entity* ed_ent;
    ARR_FOREACH_PTR(ed_map->ents, ed_ent, i) {
        bork_entity_t ent_id = ent_arr + i;
        struct bork_entity* ent = bork_entity_get(ent_id);
        bork_editor_complete_entity(ent, ed_ent);
        if(ed_ent->type < BORK_ITEM_DOGFOOD) ARR_PUSH(map->enemies, ent_id);
        else ARR_PUSH(map->items, ent_id);
    }
}


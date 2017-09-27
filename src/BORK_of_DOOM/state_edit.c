#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "upgrades.h"
#include "recycler.h"
#include "game_states.h"
#include "datapad_content.h"

static void bork_editor_write_map(struct bork_editor_map* map, char* filename);
static void bork_editor_update(struct pg_game_state* state);
static void bork_editor_draw(struct pg_game_state* state);

void bork_editor_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 60, 3);
    pg_mouse_mode(0);
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

static int bork_editor_get_hovered_ent(struct bork_editor_data* d)
{
    vec2 mouse;
    pg_mouse_pos(mouse);
    vec2_scale(mouse, mouse, 1 / d->core->screen_size[1]);
    vec2_sub(mouse, mouse, (vec2){ 0.19, 0.19 });
    vec2_scale(mouse, mouse, (1 / (0.02 * 32)) * 32);
    int i;
    struct bork_editor_entity* ent;
    float closest_dist = 1;
    int closest_idx = -1;
    ARR_FOREACH_PTR(d->map.ents, ent, i) {
        if(ent->pos[2] < d->cursor[2] || ent->pos[2] >= d->cursor[2] + 1) continue;
        vec2 ent_to_mouse;
        vec2_sub(ent_to_mouse, ent->pos, mouse);
        float dist = vec2_len(ent_to_mouse);
        if(dist < closest_dist) {
            closest_dist = dist;
            closest_idx = i;
        }
    }
    return closest_idx;
}

static void bork_editor_place_tile(struct bork_editor_data* d,
                                   struct bork_editor_tile* tile,
                                   int x, int y, int z)
{
    struct bork_editor_tile* dst = &d->map.tiles[x][y][z];
    if(dst->type == BORK_TILE_EDITOR_DOOR) {
        ARR_SWAPSPLICE(d->map.objs, dst->obj_idx, 1);
        if(dst->obj_idx < d->map.objs.len) {
            struct bork_editor_obj* dst_obj = &d->map.objs.data[dst->obj_idx];
            struct bork_editor_tile* swapped_obj_tile =
                &d->map.tiles[dst_obj->x][dst_obj->y][dst_obj->z];
            swapped_obj_tile->obj_idx = dst->obj_idx;
        }
    }
    *dst = *tile;
    if(tile->type == BORK_TILE_EDITOR_DOOR) {
        dst->obj_idx = d->map.objs.len;
        ARR_PUSH(d->map.objs, (struct bork_editor_obj){ .x = x, .y = y, .z = z });
    }
}

static void bork_editor_update_map(struct bork_editor_data* d)
{
    int ctrl = pg_check_input(SDL_SCANCODE_LCTRL, PG_CONTROL_HIT | PG_CONTROL_HELD);
    int shift = pg_check_input(SDL_SCANCODE_LSHIFT, PG_CONTROL_HIT | PG_CONTROL_HELD);
    if(pg_check_input(PG_LEFT_MOUSE, PG_CONTROL_HIT)) {
        vec2 click;
        pg_mouse_pos(click);
        vec2_scale(click, click, 1 / d->core->screen_size[1]);
        vec2_sub(click, click, (vec2){ 0.19, 0.19 });
        vec2_scale(click, click, (1 / (0.02 * 32)) * 32);
        if(!(click[0] < 0 || click[0] >= 32 || click[1] < 0 || click[1] >= 32)) {
            struct bork_editor_entity new_ent = {
                .type = d->ent_type,
                .pos = { click[0], click[1], d->cursor[2] + 0.5 } };
            if(d->ent_type == BORK_ITEM_DATAPAD) new_ent.option[0] = d->datapad_id;
            else if(d->ent_type == BORK_ITEM_UPGRADE) {
                new_ent.option[0] = d->upgrade_type[0];
                new_ent.option[1] = d->upgrade_type[1];
            }
            ARR_PUSH(d->map.ents, new_ent);
        }
    } else if(pg_check_input(PG_RIGHT_MOUSE, PG_CONTROL_HIT)) {
        int hovered = bork_editor_get_hovered_ent(d);
        if(hovered >= 0) {
            ARR_SWAPSPLICE(d->map.ents, hovered, 1);
        }
    }
    if(pg_check_input(SDL_SCANCODE_ESCAPE, PG_CONTROL_HIT)) {
        d->select_mode = MAX(d->select_mode - 1, -1);
    }
    if(pg_check_input(SDL_SCANCODE_H, PG_CONTROL_HIT)) {
        if(ctrl) {
            if(shift) {
                d->current_tile.dir |= 1 << PG_LEFT;
            } else d->current_tile.dir = 1 << PG_LEFT;
        } else if(shift) {
            d->cursor[0] = MOD(d->cursor[0] - 10, 32);
        } else {
            d->cursor[0] = MOD(d->cursor[0] - 1, 32);
        }
    }
    if(pg_check_input(SDL_SCANCODE_L, PG_CONTROL_HIT)) {
        if(ctrl) {
            if(shift) {
                d->current_tile.dir |= 1 << PG_RIGHT;
            } else d->current_tile.dir = 1 << PG_RIGHT;
        } else if(shift) {
            d->cursor[0] = MOD(d->cursor[0] + 10, 32);
        } else {
            d->cursor[0] = MOD(d->cursor[0] + 1, 32);
        }
    }
    if(pg_check_input(SDL_SCANCODE_J, PG_CONTROL_HIT)) {
        if(ctrl) {
            if(shift) {
                d->current_tile.dir |= 1 << PG_FRONT;
            } else d->current_tile.dir = 1 << PG_FRONT;
        } else if(shift) {
            d->cursor[1] = MOD(d->cursor[1] + 10, 32);
        } else {
            d->cursor[1] = MOD(d->cursor[1] + 1, 32);
        }
    }
    if(pg_check_input(SDL_SCANCODE_K, PG_CONTROL_HIT)) {
        if(ctrl) {
            if(shift) {
                d->current_tile.dir |= 1 << PG_BACK;
            } else d->current_tile.dir = 1 << PG_BACK;
        } else if(shift) {
            d->cursor[1] = MOD(d->cursor[1] - 10, 32);
        } else {
            d->cursor[1] = MOD(d->cursor[1] - 1, 32);
        }
    }
    if(pg_check_input(SDL_SCANCODE_N, PG_CONTROL_HIT)) {
        if(ctrl) d->current_tile.dir = 0;
    }
    if(pg_check_input(SDL_SCANCODE_V, PG_CONTROL_HIT)) {
        if(d->select_mode < 1) {
            d->select_mode = 1;
            d->selection[0] = d->cursor[0];
            d->selection[1] = d->cursor[1];
            d->selection[2] = d->cursor[0];
            d->selection[3] = d->cursor[1];
        } else d->select_mode = 0;
    }
    if(pg_check_input(SDL_SCANCODE_U, PG_CONTROL_HIT)) {
        if(ctrl)
            d->cursor[2] = MOD(d->cursor[2] + 1, 32);
        else if(shift)
            d->ent_type = MOD((int)d->ent_type + 1, BORK_ENTITY_TYPES);
        else d->current_tile.type = MOD((int)d->current_tile.type + 1, BORK_TILE_COUNT);
    }
    if(pg_check_input(SDL_SCANCODE_Y, PG_CONTROL_HIT)) {
        if(ctrl)
            d->cursor[2] = MOD(d->cursor[2] - 1, 32);
        else if(shift)
            d->ent_type = MOD((int)d->ent_type - 1, BORK_ENTITY_TYPES);
        else d->current_tile.type = MOD((int)d->current_tile.type - 1, BORK_TILE_COUNT);
    }
    if(pg_check_input(SDL_SCANCODE_R, PG_CONTROL_HIT)) {
        if(ctrl) bork_editor_write_map(&d->map, "test.bork_map");
        else if(shift) bork_editor_load_map(&d->map, "test.bork_map");
    }
    if(pg_check_input(SDL_SCANCODE_P, PG_CONTROL_HIT)) {
        d->current_tile.dir ^=  (1 << 7);
    }
    if(pg_check_input(SDL_SCANCODE_SPACE, PG_CONTROL_HIT)) {
        if(d->select_mode == -1) {
            bork_editor_place_tile(d, &d->current_tile, 31 - d->cursor[0], d->cursor[1], d->cursor[2]);
        } else {
            int select_rect[4] = {
                MIN(d->selection[0], d->selection[2]), MIN(d->selection[1], d->selection[3]),
                MAX(d->selection[0], d->selection[2]), MAX(d->selection[1], d->selection[3]) };
            ++select_rect[2];
            ++select_rect[3];
            int x, y;
            for(x = select_rect[0]; x < select_rect[2]; ++x) {
                for(y = select_rect[1]; y < select_rect[3]; ++y) {
                    bork_editor_place_tile(d, &d->current_tile, 31 - x, y, d->cursor[2]);
                }
            }
        }
    }
    struct bork_editor_tile* tile = &d->map.tiles[31 - d->cursor[0]][d->cursor[1]][d->cursor[2]];
    if(tile->type == BORK_TILE_EDITOR_DOOR) {
        struct bork_editor_obj* obj = &d->map.objs.data[tile->obj_idx];
        if(pg_check_input(SDL_SCANCODE_D, PG_CONTROL_HIT)) {
                obj->door.flags ^= 1;
        } else {
            int code_move = 0, code_idx = 0;
            if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)) {
                code_move = 1;
            } else if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)) {
                code_move = -1;
            }
            if(code_move) {
                if(ctrl) code_idx += 1;
                if(shift) code_idx += 2;
                obj->door.code[code_idx] = MOD((int)obj->door.code[code_idx] + code_move, 10);
            }
        }
    } else if(d->ent_type == BORK_ITEM_DATAPAD) {
        if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)) {
            d->datapad_id = MOD(d->datapad_id + 1, NUM_DATAPADS);
        } else if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)) {
            d->datapad_id = MOD(d->datapad_id - 1, NUM_DATAPADS);
        }
    } else if(d->ent_type == BORK_ITEM_UPGRADE) {
        int t = 0;
        if(pg_check_input(SDL_SCANCODE_LSHIFT, PG_CONTROL_HELD)) t = 1;
        if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT)) {
            d->upgrade_type[t] = MOD(d->upgrade_type[t] + 1, BORK_NUM_UPGRADES);
        } else if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT)) {
            d->upgrade_type[t] = MOD(d->upgrade_type[t] - 1, BORK_NUM_UPGRADES);
        }
    }
    if(d->select_mode == 1) {
        d->selection[2] = d->cursor[0];
        d->selection[3] = d->cursor[1];
    }
    pg_flush_input();
}

static void bork_editor_update(struct pg_game_state* state)
{
    struct bork_editor_data* d = state->data;
    pg_poll_input();
    if(pg_user_exit()) state->running = 0;
    bork_editor_update_map(d);
}

static void bork_editor_draw_items(struct bork_editor_data* d)
{
    struct pg_shader* shader = &d->core->shader_2d;
    pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_2d_texture(shader, &d->core->bullet_tex);
    pg_shader_2d_tex_frame(shader, 2);
    struct pg_model* model = &d->core->quad_2d_ctr;
    pg_model_begin(model, shader);
    int i;
    struct bork_editor_entity* ent;
    ARR_FOREACH_PTR(d->map.ents, ent, i) {
        if(!(ent->pos[2] >= d->cursor[2] && ent->pos[2] <= d->cursor[2] + 1))
            continue;
        vec2 pos = { ent->pos[0] / 32.0f * (0.02 * 32.0f / 1.0f) + 0.19,
                     ent->pos[1] / 32.0f * (0.02 * 32.0f / 1.0f) + 0.19 };
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
    pg_model_begin(&d->core->quad_2d_ctr, shader);
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
            struct bork_editor_tile* tile = &d->map.tiles[31 - x][y][d->cursor[2]];
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
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
    }
    /*  Get the stuff for the currently selected tile   */
    struct bork_editor_tile* tile = &d->map.tiles[31 - d->cursor[0]][d->cursor[1]][d->cursor[2]];
    const struct bork_tile_detail* detail = bork_tile_detail(tile->type);
    const struct bork_tile_detail* cursor_detail = bork_tile_detail(d->current_tile.type);
    /*  Draw the scaled-up view of the currently selected tile in the map   */
    pg_shader_2d_tex_frame(&d->core->shader_2d, d->current_tile.type);
    pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 1, 1, 1, 1 });
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ d->cursor[0] * 0.02 + 0.2, d->cursor[1] * 0.02 + 0.2 },
        (vec2){ 0.02, 0.02 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    if(cursor_detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(d->current_tile.dir & (1 << i))) continue;
            pg_shader_2d_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
    }
    /*  Draw the current "brush" tile   */
    pg_shader_2d_tex_frame(&d->core->shader_2d, tile->type);
    pg_shader_2d_transform(&d->core->shader_2d,
        (vec2){ 0.95, 0.2 }, (vec2){ 0.03, 0.03 }, 0);
    pg_model_draw(&d->core->quad_2d_ctr, NULL);
    if(detail->tile_flags & BORK_TILE_HAS_ORIENTATION) {
        int i;
        for(i = 0; i < 4; ++i) {
            if(!(tile->dir & (1 << i))) continue;
            pg_shader_2d_tex_frame(&d->core->shader_2d, 252 + i);
            pg_model_draw(&d->core->quad_2d_ctr, NULL);
        }
    }
    bork_editor_draw_items(d);
    /*  Text UI */
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ d->core->aspect_ratio, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    /*  Right-side UI text  */
    struct pg_shader_text text = { .use_blocks = 5 };
    snprintf(text.block[0], 64, "LEVEL: %d", d->cursor[2]);
    vec4_set(text.block_style[0], 0.25, 0.1, 0.02, 1.2);
    vec4_set(text.block_color[0], 1, 1, 1, 1);
    snprintf(text.block[1], 64, "TILE DETAIL: %s %s", detail->name,
        (tile->type >= BORK_TILE_EDITOR_DOOR && (tile->dir & (1 << 7)))
            ? "(PUSH)" : "");
    vec4_set(text.block_style[1], 1, 0.2, 0.02, 1.2);
    vec4_set(text.block_color[1], 1, 1, 1, 1);
    snprintf(text.block[2], 64, "PLACING TILE: %s %s", cursor_detail->name,
        (d->current_tile.type >= BORK_TILE_EDITOR_DOOR && (d->current_tile.dir & (1 << 7)))
            ? "(PUSH)" : "");
    vec4_set(text.block_style[2], 1, 0.23, 0.02, 1.2);
    vec4_set(text.block_color[2], 1, 1, 1, 1);
    /*  Entity text */
    int hovered_ent = bork_editor_get_hovered_ent(d);
    const struct bork_entity_profile* prof = NULL;
    if(hovered_ent >= 0) {
        prof = &BORK_ENT_PROFILES[d->map.ents.data[hovered_ent].type];
    }
    snprintf(text.block[3], 64, "ENTITY: %s", prof ? prof->name : "NONE");
    vec4_set(text.block_style[3], 1, 0.26, 0.02, 1.2);
    vec4_set(text.block_color[3], 1, 1, 1, 1);
    prof = &BORK_ENT_PROFILES[d->ent_type];
    snprintf(text.block[4], 64, "PLACING ENTITY: %s", prof->name);
    vec4_set(text.block_style[4], 1, 0.29, 0.02, 1.2);
    vec4_set(text.block_color[4], 1, 1, 1, 1);
    if(d->ent_type == BORK_ITEM_DATAPAD) {
        int b = text.use_blocks;
        snprintf(text.block[b], 64, "DATAPAD ID: %d", d->datapad_id);
        vec4_set(text.block_style[b], 1, 0.32, 0.02, 1.2);
        vec4_set(text.block_color[b], 1, 1, 1, 1);
        ++b;
        snprintf(text.block[b], 64, "  %s", BORK_DATAPADS[d->datapad_id].title);
        vec4_set(text.block_style[b], 1, 0.35, 0.02, 1.2);
        vec4_set(text.block_color[b], 1, 1, 1, 1);
        text.use_blocks += 2;
    } else if(d->ent_type == BORK_ITEM_UPGRADE) {
        int b = text.use_blocks;
        const struct bork_upgrade_detail* up_d = bork_upgrade_detail(d->upgrade_type[0]);
        snprintf(text.block[b], 64, "UPGRADE 1: %s", up_d->name);
        vec4_set(text.block_style[b], 1, 0.32, 0.02, 1.2);
        vec4_set(text.block_color[b], 1, 1, 1, 1);
        ++b;
        up_d = bork_upgrade_detail(d->upgrade_type[1]);
        snprintf(text.block[b], 64, "UPGRADE 2: %s", up_d->name);
        vec4_set(text.block_style[b], 1, 0.35, 0.02, 1.2);
        vec4_set(text.block_color[b], 1, 1, 1, 1);
        text.use_blocks += 2;
    }
    if(tile->type == BORK_TILE_EDITOR_DOOR) {
        int b = text.use_blocks;
        struct bork_editor_obj* obj = &d->map.objs.data[tile->obj_idx];
        if(obj->door.flags & 1) {
            snprintf(text.block[b], 64, "DOOR CODE: %u%u%u%u",
                obj->door.code[0], obj->door.code[1],
                obj->door.code[2], obj->door.code[3]);
        } else {
            snprintf(text.block[b], 64, "NOT LOCKED");
        }
        vec4_set(text.block_style[b], 1, 0.17, 0.02, 1.2);
        vec4_set(text.block_color[b], 1, 1, 1, 1);
        ++text.use_blocks;
    }
    pg_shader_text_write(shader, &text);
    bork_editor_draw_items_text(d);
    bork_draw_fps(d->core);
}

static void bork_editor_write_map(struct bork_editor_map* map, char* filename)
{
    FILE* file = fopen(filename, "wb");
    if(!file) {
        printf("BORK map writing error: could not open file %s\n", filename);
    }
    fwrite(map->tiles, sizeof(struct bork_editor_tile), 32 * 32 *32, file);
    uint32_t len = map->objs.len;
    fwrite(&len, sizeof(len), 1, file);
    fwrite(map->objs.data, sizeof(struct bork_editor_obj), len, file);
    len = map->ents.len;
    fwrite(&len, sizeof(len), 1, file);
    fwrite(map->ents.data, sizeof(struct bork_editor_entity), len, file);
    fclose(file);
}

int bork_editor_load_map(struct bork_editor_map* map, char* filename)
{
    *map = (struct bork_editor_map){};
    FILE* file = fopen(filename, "rb");
    if(!file) {
        printf("BORK map loading error: could not open file %s\n", filename);
        return 0;
    }
    int r = fread(map->tiles, sizeof(struct bork_editor_tile), 32 * 32 *32, file);
    if(r != 32 * 32 * 32) {
        printf("WARNING! Map file did not contain the correct number of tiles!\n");
    }
    uint32_t num_objs = 0;
    r = fread(&num_objs, sizeof(num_objs), 1, file);
    ARR_RESERVE_CLEAR(map->objs, num_objs);
    r = fread(map->objs.data, sizeof(struct bork_editor_obj), num_objs, file);
    if(r != num_objs) {
        printf("WARNING! Map file did not contain the correct number of map objects!\n");
    }
    map->objs.len = r;
    uint32_t num_items = 0;
    r = fread(&num_items, sizeof(num_items), 1, file);
    ARR_RESERVE(map->ents, num_items);
    r = fread(map->ents.data, sizeof(struct bork_editor_entity), num_items, file);
    if(r != num_items) {
        printf("WARNING! Map file's item count did not match actual number of stored items!\n");
    }
    map->ents.len = r;
    fclose(file);
    return 1;
}

void bork_editor_complete_entity(struct bork_entity* ent,
                                 struct bork_editor_entity* ed)
{
    *ent = (struct bork_entity) {
        .pos = { (32 - ed->pos[0]) * 2, ed->pos[1] * 2, ed->pos[2] * 2 },
        .flags = BORK_ENT_PROFILES[ed->type].base_flags,
        .HP = BORK_ENT_PROFILES[ed->type].base_hp,
        .item_quantity = 1,
        .type = ed->type,
    };
    if(ed->type == BORK_ITEM_DATAPAD) ent->counter[0] = ed->option[0];
    else if(ed->type == BORK_ITEM_UPGRADE) {
        ent->counter[0] = ed->option[0];
        ent->counter[1] = ed->option[1];
    }
}

void bork_editor_complete_door(struct bork_map* map, struct bork_editor_map* ed_map,
                               int x, int y, int z)
{
    vec3 tile_ctr = { x * 2 + 1, y * 2 + 1, z * 2 + 1 };
    quat door_dir, pad_dir;
    quat_identity(door_dir);
    quat_identity(pad_dir);
    struct bork_editor_tile* ed_tile = &ed_map->tiles[x][y][z];
    if(ed_tile->dir & ((1 << PG_LEFT) | (1 << PG_RIGHT)))
        quat_rotate(door_dir, M_PI * 0.5, (vec3){ 0, 0, -1 });
    else quat_rotate(pad_dir, M_PI * 0.5, (vec3){ 0, 0, -1 });
    struct bork_editor_obj* ed_obj = &ed_map->objs.data[ed_tile->obj_idx];
    struct bork_map_object new_obj = { .type = BORK_MAP_DOOR,
        .door = { .locked = (ed_obj->door.flags & 1),
                  .code = { ed_obj->door.code[0], ed_obj->door.code[1],
                            ed_obj->door.code[2], ed_obj->door.code[3] } },
        .pos = { tile_ctr[0], tile_ctr[1], tile_ctr[2] },
        .dir = { door_dir[0], door_dir[1], door_dir[2], door_dir[3] } };
    vec3 pad0_pos, pad1_pos = { 0.7, 0.975, 0 };
    quat_mul_vec3(pad1_pos, pad_dir, pad1_pos);
    vec3_sub(pad0_pos, tile_ctr, pad1_pos);
    vec3_add(pad1_pos, tile_ctr, pad1_pos);
    ARR_PUSH(map->doors, new_obj);
    new_obj = (struct bork_map_object){ .type = BORK_MAP_DOORPAD,
        .pos = { pad0_pos[0], pad0_pos[1], pad0_pos[2] },
        .dir = { pad_dir[0], pad_dir[1], pad_dir[2], pad_dir[3] },
        .doorpad = { .door_idx = map->doors.len - 1 } };
    ARR_PUSH(map->doorpads, new_obj);
    new_obj = (struct bork_map_object){ .type = BORK_MAP_DOORPAD,
        .pos = { pad1_pos[0], pad1_pos[1], pad1_pos[2] },
        .dir = { pad_dir[0], pad_dir[1], pad_dir[2], pad_dir[3] },
        .doorpad = { .door_idx = map->doors.len - 1 } };
    ARR_PUSH(map->doorpads, new_obj);
}

void bork_editor_complete_map(struct bork_map* map, struct bork_editor_map* ed_map)
{
    int i, j, k;
    for(i = 0; i < 32; ++i) {
        for(j = 0; j < 32; ++j) {
            for(k = 0; k < 32; ++k) {
                struct bork_tile* tile = &map->data[i][j][k];
                struct bork_editor_tile* ed_tile = &ed_map->tiles[i][j][k];
                *tile = (struct bork_tile) {
                    .type = ed_tile->type,
                    .orientation = ed_tile->dir & 0x0F };
                if(tile->type == BORK_TILE_EDITOR_DOOR) {
                    bork_editor_complete_door(map, ed_map, i, j, k);
                    tile->type = BORK_TILE_ATMO;
                } else if(tile->type == BORK_TILE_EDITOR_LIGHT1) {
                    struct pg_light new_light;
                    pg_light_spotlight(&new_light,
                        (vec3){ i * 2 + 1, j * 2 + 1, k * 2 + 1.75 }, 9,
                        (vec3){ 1.5, 1.5, 1.2 }, (vec3){ 0, 0, -1 }, 1.7);
                    int push = 0;
                    if(ed_tile->dir & (1 << 7)) {
                        push = 1;
                        new_light.pos[2] += 1.3;
                    }
                    ARR_PUSH(map->spotlights, new_light);
                    struct bork_map_light_fixture new_lfix = {
                        .pos = { new_light.pos[0], new_light.pos[1], new_light.pos[2] - 0.2 },
                        .type = 0 + (push ? 2 : 0),
                    };
                    ARR_PUSH(map->light_fixtures, new_lfix);
                    tile->type = BORK_TILE_ATMO;
                } else if(tile->type == BORK_TILE_EDITOR_LIGHT_WALLMOUNT) {
                    vec3 dir_angle = { 0, 0, -1 };
                    vec3 pos = { i * 2 + 1, j * 2 + 1, k * 2 + 1.75 };
                    if(ed_tile->dir & (1 << PG_FRONT)) {
                        dir_angle[1] = 1;
                        pos[1] -= 0.8;
                    }
                    if(ed_tile->dir & (1 << PG_BACK)) {
                        dir_angle[1] = -1;
                        pos[1] += 0.8;
                    }
                    if(ed_tile->dir & (1 << PG_LEFT)) {
                        dir_angle[0] = 1;
                        pos[0] -= 0.8;
                    }
                    if(ed_tile->dir & (1 << PG_RIGHT)) {
                        dir_angle[0] = -1;
                        pos[0] += 0.8;
                    }
                    int push = 0;
                    if(ed_tile->dir & (1 << 7)) {
                        push = 1;
                        pos[2] += 1.3;
                    }
                    vec3_normalize(dir_angle, dir_angle);
                    struct pg_light new_light;
                    pg_light_spotlight(&new_light,
                        pos, 7, (vec3){ 1.5, 1.5, 1.2 }, dir_angle, 1.15);
                    ARR_PUSH(map->spotlights, new_light);
                    struct bork_map_light_fixture new_lfix = {
                        .pos = { new_light.pos[0], new_light.pos[1], new_light.pos[2] - 0.1 },
                        .type = 1 + (push ? 2 : 0),
                    };
                    ARR_PUSH(map->light_fixtures, new_lfix);
                    tile->type = BORK_TILE_ATMO;
                } else if(tile->type == BORK_TILE_EDITOR_LIGHT_SMALLMOUNT) {
                    vec3 dir_angle = { 0, 0, -1 };
                    vec3 pos = { i * 2 + 1, j * 2 + 1, k * 2 + 1 };
                    if(ed_tile->dir & (1 << PG_FRONT)) {
                        dir_angle[1] = 0.3;
                        pos[1] -= 0.8;
                    }
                    if(ed_tile->dir & (1 << PG_BACK)) {
                        dir_angle[1] = -0.3;
                        pos[1] += 0.8;
                    }
                    if(ed_tile->dir & (1 << PG_LEFT)) {
                        dir_angle[0] = 0.3;
                        pos[0] -= 0.8;
                    }
                    if(ed_tile->dir & (1 << PG_RIGHT)) {
                        dir_angle[0] = -0.3;
                        pos[0] += 0.8;
                    }
                    int push = 0;
                    if(ed_tile->dir & (1 << 7)) {
                        push = 1;
                        pos[2] += 1.3;
                    }
                    vec3_normalize(dir_angle, dir_angle);
                    struct pg_light new_light;
                    pg_light_spotlight(&new_light,
                        pos, 3.5, (vec3){ 1.5, 1.5, 1.2 }, dir_angle, 0.9);
                    ARR_PUSH(map->spotlights, new_light);
                    struct bork_map_light_fixture new_lfix = {
                        .pos = { new_light.pos[0], new_light.pos[1], new_light.pos[2] - 0.1 },
                        .type = 1 + (push ? 2 : 0),
                    };
                    ARR_PUSH(map->light_fixtures, new_lfix);
                    tile->type = BORK_TILE_ATMO;
                }
            }
        }
    }
    bork_map_calc_travel(map);
    bork_entity_t ent_arr = bork_entity_new(ed_map->ents.len);
    struct bork_editor_entity* ed_ent;
    ARR_FOREACH_PTR(ed_map->ents, ed_ent, i) {
        if(ed_ent->type == BORK_ENTITY_PLAYER) {
            if(!map->plr) continue;
            vec3_set(map->plr->pos,
                (32 - ed_ent->pos[0]) * 2, ed_ent->pos[1] * 2, ed_ent->pos[2] * 2);
            continue;
        }
        bork_entity_t ent_id = ent_arr + i;
        struct bork_entity* ent = bork_entity_get(ent_id);
        bork_editor_complete_entity(ent, ed_ent);
        if(ed_ent->type == BORK_ENTITY_ENEMY) bork_map_add_enemy(map, ent_id);
        else bork_map_add_item(map, ent_id);
    }
}


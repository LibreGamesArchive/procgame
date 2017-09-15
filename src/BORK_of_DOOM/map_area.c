#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"
#include "game_states.h"

static const char* BORK_AREA_STRING[] = {
    [BORK_AREA_PETS] = "P.E.T.S.",
    [BORK_AREA_WAREHOUSE] = "WAREHOUSE",
    [BORK_AREA_CAFETERIA] = "CAFETERIA",
    [BORK_AREA_REC_CENTER] = "DOG PARK",
    [BORK_AREA_INFIRMARY] = "VETERINARY CLINIC",
    [BORK_AREA_SCIENCE_LABS] = "SCIENCE LABS",
    [BORK_AREA_COMMAND] = "COMMAND CENTER",
    [BORK_AREA_MUTT] = "M.U.T.T.",
    [BORK_AREA_EXTERIOR] = "BONZ EXTERIOR",
};
const char* bork_map_area_str(enum bork_area area) {
    if(area >= BORK_AREA_PETS && area <= BORK_AREA_MUTT) return BORK_AREA_STRING[area];
    else return BORK_AREA_STRING[BORK_AREA_EXTERIOR];
}

static int tile_model_basic(struct bork_map*, struct pg_texture*,
                            struct bork_tile*, int, int, int);
static int tile_model_duct(struct bork_map*, struct pg_texture*,
                           struct bork_tile*, int, int, int);
static int tile_model_ramp(struct bork_map*, struct pg_texture*,
                           struct bork_tile*, int, int, int);
static void bork_map_generate_model(struct bork_map* map,
                                    struct pg_texture* env_atlas);

/*  Tile details */
struct bork_tile_detail BORK_TILE_DETAILS[] = {
    [BORK_TILE_VAC] = { .name = "VACUUM",
        .face_flags = { BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL,
                        BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL } },
    [BORK_TILE_ATMO] = { .name = "ATMOSPHERE",
        .face_flags = { BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL,
                        BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL } },
    [BORK_TILE_HULL] = { .name = "BASIC HULL",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [PG_LEFT] = 19, [PG_RIGHT] = 19,
            [PG_FRONT] = 19, [PG_BACK] = 19,
            [PG_TOP] = 3, [PG_DOWN] = 20 },
        .add_model = tile_model_basic },
    [BORK_TILE_HULL_EDGE] = { .name = "HULL EDGE",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [PG_LEFT] = 35, [PG_RIGHT] = 35,
            [PG_FRONT] = 35, [PG_BACK] = 35,
            [PG_TOP] = 3, [PG_DOWN] = 20 },
        .add_model = tile_model_basic },
    [BORK_TILE_CONTROL_PANEL] = { .name = "CONTROL PANEL",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [PG_LEFT] = 4, [PG_RIGHT] = 4,
            [PG_FRONT] = 4, [PG_BACK] = 4,
            [PG_TOP] = 3, [PG_DOWN] = 3 },
        .add_model = tile_model_basic },
    [BORK_TILE_LADDER] = { .name = "LADDER",
        .tile_flags = BORK_TILE_HAS_ORIENTATION | BORK_TILE_FACE_ORIENTED,
        .face_flags = { BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_FLUSH_SURFACE |
                            BORK_FACE_HAS_BACKFACE | BORK_FACE_SEETHRU_SURFACE |
                            BORK_FACE_TRAVEL_ORIENT_OPP, 0, 0 },
        .face_inset = { 0.1, 0.1, 0.1, 0.1 },
        .tex_tile = {
            [PG_LEFT] = 5, [PG_RIGHT] = 5,
            [PG_FRONT] = 5, [PG_BACK] = 5 },
        .add_model = tile_model_basic },
    [BORK_TILE_CATWALK] = { .name = "CATWALK",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = { BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL,
                        [PG_BOTTOM] = BORK_FACE_TRAVEL,
                        [PG_TOP] = BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE },
        .face_inset = { },
        .tex_tile = { [PG_TOP] = 6 },
        .add_model = tile_model_basic },
    [BORK_TILE_HANDRAIL] = { .name = "HANDRAIL",
        .tile_flags = BORK_TILE_HAS_ORIENTATION | BORK_TILE_FACE_ORIENTED,
        .face_flags = { BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_TRAVEL, BORK_FACE_TRAVEL },
        .face_inset = { -0.05, -0.05, -0.05, -0.05 },
        .tex_tile = { 7, 7, 7, 7 },
        .add_model = tile_model_basic },
    [BORK_TILE_HANDRAIL_TOP] = { .name = "HANDRAIL TOP",
        .tile_flags = BORK_TILE_HAS_ORIENTATION | BORK_TILE_FACE_ORIENTED,
        .face_flags = { BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_TOP |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_TOP |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_TOP |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE |
                            BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_TOP |
                            BORK_FACE_TRAVEL_ORIENT_OPP,
                        BORK_FACE_TRAVEL, BORK_FACE_TRAVEL },
        .face_inset = { -0.05, -0.05, -0.05, -0.05 },
        .tex_tile = { 7, 7, 7, 7 },
        .add_model = tile_model_basic },
    [BORK_TILE_POWERBLOCK] = { .name = "POWER BLOCK",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = { 1, 1, 1, 1, 1, 1 },
        .tex_tile = {
            [PG_LEFT] = 8, [PG_RIGHT] = 8,
            [PG_FRONT] = 8, [PG_BACK] = 8,
            [PG_TOP] = 9, [PG_DOWN] = 9 },
        .add_model = tile_model_basic },
    [BORK_TILE_TABLE] = { .name = "TABLE",
        .face_flags = {
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            [PG_TOP] = BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HAS_BACKFACE,
            [PG_BOTTOM] = 0 },
        .face_inset = { [PG_TOP] = 0.5 },
        .tex_tile = { 11, 11, 11, 11, 10, 10 },
        .add_model = tile_model_basic },
    [BORK_TILE_GARDEN] = { .name = "GARDEN",
        .face_flags = {
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM,
            [PG_TOP] = BORK_FACE_HAS_SURFACE,
            [PG_BOTTOM] = BORK_FACE_HAS_SURFACE },
        .face_inset = { 0, 0, 0, 0, [PG_TOP] = 0.5 },
        .tex_tile = { 12, 12, 12, 12, 13, 20 },
        .add_model = tile_model_basic },
    [BORK_TILE_CATWALK_HALF] = { .name = "CATWALK HALF",
        .tile_flags = BORK_TILE_TRAVEL_DROP |
                        BORK_TILE_HAS_ORIENTATION | BORK_TILE_FACE_ORIENTED,
        .face_flags = {
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            BORK_FACE_HAS_SURFACE | BORK_FACE_HAS_BACKFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            [PG_TOP] = BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HAS_BACKFACE,
            [PG_BOTTOM] = 0, },
        .face_inset = { 0, 0, 0, 0, [PG_TOP] = 0.5 },
        .tex_tile = { 14, 14, 14, 14, [PG_TOP] = 6 },
        .add_model = tile_model_basic },
    [BORK_TILE_HULL_HALF] = { .name = "HULL HALF",
        .tile_flags = BORK_TILE_TRAVEL_DROP,
        .face_flags = {
            BORK_FACE_HAS_SURFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            BORK_FACE_HAS_SURFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            BORK_FACE_HAS_SURFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            BORK_FACE_HAS_SURFACE | BORK_FACE_NO_SELF_OPPOSITE |
                BORK_FACE_SEETHRU_SURFACE | BORK_FACE_HALF_BOTTOM | BORK_FACE_TRAVEL,
            [PG_TOP] = BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE | BORK_FACE_TRAVEL,
            [PG_BOTTOM] = BORK_FACE_HAS_SURFACE, },
        .face_inset = { 0, 0, 0, 0, [PG_TOP] = 0.5 },
        .tex_tile = { 15, 15, 15, 15, [PG_TOP] = 3 },
        .add_model = tile_model_basic },
    [BORK_TILE_DUCT] = { .name = "DUCT-WORK",
        .tile_flags = BORK_TILE_HAS_ORIENTATION,
        .face_flags = { 1, 1, 1, 1, [PG_TOP] = 0, [PG_BOTTOM] = 1 },
        .add_model = tile_model_duct },
    [BORK_TILE_CARGO_RED] = { .name = "CARGO (RED)",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = {
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            [PG_TOP] = BORK_FACE_HAS_SURFACE,
            [PG_BOTTOM] = BORK_FACE_FORCE_SURFACE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE },
        .face_inset = { 0, 0, 0, 0, [PG_TOP] = 0, [PG_BOTTOM] = 0.1 },
        .tex_tile = { 22, 22, 22, 22, [PG_TOP] = 23, [PG_BOTTOM] = 37 },
        .add_model = tile_model_basic },
    [BORK_TILE_CARGO_BLUE] = { .name = "CARGO (BLUE)",
        .tile_flags = BORK_TILE_WALK_ABOVE,
        .face_flags = {
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            BORK_FACE_NO_SELF_OPPOSITE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE,
            [PG_TOP] = BORK_FACE_HAS_SURFACE,
            [PG_BOTTOM] = BORK_FACE_FORCE_SURFACE | BORK_FACE_HAS_SURFACE | BORK_FACE_SEETHRU_SURFACE },
        .face_inset = { 0, 0, 0, 0, [PG_TOP] = 0, [PG_BOTTOM] = 0.1 },
        .tex_tile = { 38, 38, 38, 38, [PG_TOP] = 39, [PG_BOTTOM] = 37 },
        .add_model = tile_model_basic },
    [BORK_TILE_RAMP_BOTTOM] = { .name = "RAMP BOTTOM",
        .tile_flags = BORK_TILE_WALK_ABOVE | BORK_TILE_TRAVEL_DROP | BORK_TILE_HAS_ORIENTATION,
        .face_flags = { BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL },
        .add_model = tile_model_ramp },
    [BORK_TILE_RAMP_TOP] = { .name = "RAMP TOP",
        .tile_flags = BORK_TILE_WALK_ABOVE | BORK_TILE_HAS_ORIENTATION,
        .face_flags = { BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL, BORK_FACE_TRAVEL },
        .add_model = tile_model_ramp },
    [BORK_TILE_EDITOR_DOOR] = { .name = "DOOR",
        .tile_flags = BORK_TILE_HAS_ORIENTATION },
    [BORK_TILE_EDITOR_LIGHT1] = { .name = "CEIL LIGHT" },
    [BORK_TILE_EDITOR_LIGHT_WALLMOUNT] = { .name = "WALL LIGHT",
        .tile_flags = BORK_TILE_HAS_ORIENTATION },
    [BORK_TILE_EDITOR_LIGHT_SMALLMOUNT] = { .name = "SMALL LIGHT",
        .tile_flags = BORK_TILE_HAS_ORIENTATION },
};

const struct bork_tile_detail* bork_tile_detail(enum bork_tile_type type)
{
    if(type >= BORK_TILE_COUNT) return &BORK_TILE_DETAILS[BORK_TILE_VAC];
    return &BORK_TILE_DETAILS[type];
}

/*  Public interface    */
void bork_map_init(struct bork_map* map)
{
    *map = (struct bork_map){};
}

void bork_map_init_model(struct bork_map* map, struct bork_game_core* core)
{
    /*  Generate the map model  */
    pg_model_init(&map->model);
    bork_map_generate_model(map, &core->env_atlas);
    pg_shader_buffer_model(&core->shader_3d, &map->model);
    /*  And the door model  */
    pg_model_init(&map->door_model);
    vec4 face_uv[6] = {};
    pg_texture_get_frame(&core->env_atlas, 2, face_uv[PG_FRONT]);
    pg_texture_frame_flip(face_uv[PG_BACK], face_uv[PG_FRONT], 0, 1);
    pg_texture_get_frame(&core->env_atlas, 18, face_uv[PG_TOP]);
    pg_texture_frame_tx(face_uv[PG_TOP], face_uv[PG_TOP],
                        (vec2){ 1, 0.125 }, (vec2){ 0, 0 });
    pg_texture_frame_flip(face_uv[PG_BOTTOM], face_uv[PG_TOP], 0, 1);
    pg_texture_get_frame(&core->env_atlas, 2, face_uv[PG_LEFT]);
    pg_texture_frame_tx(face_uv[PG_LEFT], face_uv[PG_LEFT],
                        (vec2){ 0.125, 1 }, (vec2){ -4.0f / 512.0f, 0 });
    pg_texture_frame_flip(face_uv[PG_RIGHT], face_uv[PG_LEFT], 0, 1);
    pg_model_rect_prism(&map->door_model, (vec3){ 1, 0.125, 1 }, face_uv);
    pg_model_precalc_ntb(&map->door_model);
    pg_shader_buffer_model(&core->shader_3d, &map->door_model);
}

void bork_map_deinit(struct bork_map* map)
{
    pg_model_deinit(&map->model);
    pg_model_deinit(&map->door_model);
    ARR_DEINIT(map->doors);
    ARR_DEINIT(map->light_fixtures);
    ARR_DEINIT(map->lights);
    ARR_DEINIT(map->spotlights);
}

void bork_map_update(struct bork_map* map, struct bork_entity* plr)
{
    struct bork_map_object* obj;
    int i;
    ARR_FOREACH_PTR(map->doors, obj, i) {
        if(obj->door.open) {
            obj->door.pos += 0.1;
            if(obj->door.pos > 1.9) obj->door.pos = 1.9;
        } else if(obj->door.pos > 0) {
            obj->door.pos -= 0.1;
            if(obj->door.pos < 0) obj->door.pos = 0;
        }
    }
}

void bork_map_draw(struct bork_map* map, struct bork_game_core* core)
{
    struct pg_shader* shader = &core->shader_3d;
    pg_shader_begin(shader, &core->view);
    /*  Draw the base level geometry    */
    pg_model_begin(&map->model, shader);
    mat4 model_transform;
    mat4_translate(model_transform, 0, 0, 0);
    pg_model_draw(&map->model, model_transform);
    /*  Then draw the map doors (except lights)   */
    int i;
    struct bork_map_object* obj;
    pg_model_begin(&map->door_model, shader);
    ARR_FOREACH_PTR(map->doors, obj, i) {
        mat4_translate(model_transform, obj->pos[0], obj->pos[1], obj->pos[2] + obj->door.pos);
        mat4_mul_quat(model_transform, model_transform, obj->dir);
        if(!obj->door.locked) {
            pg_shader_3d_tex_transform(shader, (vec2){ 1, 1 },
                                       (vec2){ 0, 96.0f / 512.0f });
        } else {
            pg_shader_3d_tex_transform(shader, (vec2){ 1, 1 }, (vec2){});
        }
        pg_model_draw(&map->door_model, model_transform);
    }
    ARR_FOREACH_PTR(map->doorpads, obj, i) {
        mat4_translate(model_transform, obj->pos[0], obj->pos[1], obj->pos[2]);
        mat4_scale_aniso(model_transform, model_transform, 0.2, 0.2, 0.2);
        mat4_mul_quat(model_transform, model_transform, obj->dir);
        struct bork_map_object* door = &map->doors.data[obj->doorpad.door_idx];
        if(!door->door.locked) {
            pg_shader_3d_tex_transform(shader, (vec2){ 1, 1 },
                                       (vec2){ 0, 144.0f / 512.0f });
        } else {
            pg_shader_3d_tex_transform(shader, (vec2){ 1, 1 },
                                       (vec2){ 0, 48.0f / 512.0f });
        }
        pg_model_draw(&map->door_model, model_transform);
    }
    shader = &core->shader_sprite;
    pg_shader_begin(shader, &core->view);
    pg_shader_sprite_mode(shader, PG_SPRITE_CYLINDRICAL);
    pg_shader_sprite_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    pg_shader_sprite_texture(shader, &core->env_atlas);
    pg_shader_sprite_tex_frame(shader, 160);
    pg_model_begin(&core->enemy_model, shader);
    int current_type = 0;
    struct bork_map_light_fixture* lfix;
    ARR_FOREACH_PTR(map->light_fixtures, lfix, i) {
        mat4_translate(model_transform, lfix->pos[0], lfix->pos[1], lfix->pos[2]);
        if(lfix->type != current_type) {
            pg_shader_sprite_tex_frame(shader, lfix->type + 160);
            current_type = lfix->type;
        }
        pg_model_draw(&core->enemy_model, model_transform);
    }
}

struct bork_tile* bork_map_tile_ptr(struct bork_map* map, vec3 const pos)
{
    int x = (int)(pos[0] * 0.5f);
    int y = (int)(pos[1] * 0.5f);
    int z = (int)(pos[2] * 0.5f);
    if(x >= 0 && x < 32 && y >= 0 && y < 32 && z >= 0 && z < 32)
        return &map->data[x][y][z];
    else return NULL;
}

struct bork_tile* bork_map_tile_ptri(struct bork_map* map, int x, int y, int z)
{
    if(x >= 0 && x < 32 && y >= 0 && y < 32 && z >= 0 && z < 32)
        return &map->data[x][y][z];
    else return NULL;
}

int bork_map_check_ellipsoid(struct bork_map* map, vec3 const pos, vec3 const r)
{
    box bbox;
    vec3 r_scaled = { r[0] * 1.25, r[1] * 1.25, r[2] * 1.25 };
    vec3_sub(bbox[0], pos, r_scaled);
    vec3_add(bbox[1], pos, r_scaled);
    box_mul_vec3(bbox, bbox, (vec3){ 0.5, 0.5, 0.5 });
    int check[2][3] = { { (int)bbox[0][0], (int)bbox[0][1], (int)bbox[0][2] },
                        { (int)bbox[1][0], (int)bbox[1][1], (int)bbox[1][2] } };
    /*  First check collisions against the map  */
    struct pg_model* model = &map->model;
    int x, y, z;
    for(z = check[0][2]; z <= check[1][2]; ++z) {
        for(y = check[0][1]; y <= check[1][1]; ++y) {
            for(x = check[0][0]; x <= check[1][0]; ++x) {
                /*  Get the map area that the checked box is in */
                /*  Get a pointer to the tile in the map area   */
                struct bork_tile* tile = bork_map_tile_ptri(map, x, y, z);
                /*  If the tile is outside the map, or the tile is not
                    collidable, move on to the next one */
                if(!tile || tile->type < 2) continue;
                /*  Otherwise test collisions against this tile's associated
                    triangles in the area model */
                vec3 tile_push;
                int c = pg_model_collide_ellipsoid_sub(model, tile_push,
                            pos, r, 1, tile->model_tri_idx, tile->num_tris);
                if(c >= 0) return 1;
            }
        }
    }
    return 0;
}

int bork_map_check_sphere(struct bork_map* map, vec3 const pos, float r)
{
    box bbox;
    vec3 r_scaled = { r * 1.25, r * 1.25, r * 1.25 };
    vec3_sub(bbox[0], pos, r_scaled);
    vec3_add(bbox[1], pos, r_scaled);
    box_mul_vec3(bbox, bbox, (vec3){ 0.5, 0.5, 0.5 });
    int check[2][3] = { { (int)bbox[0][0], (int)bbox[0][1], (int)bbox[0][2] },
                        { (int)bbox[1][0], (int)bbox[1][1], (int)bbox[1][2] } };
    /*  First check collisions against the map  */
    struct pg_model* model = &map->model;
    int x, y, z;
    for(z = check[0][2]; z <= check[1][2]; ++z) {
        for(y = check[0][1]; y <= check[1][1]; ++y) {
            for(x = check[0][0]; x <= check[1][0]; ++x) {
                /*  Get the map area that the checked box is in */
                /*  Get a pointer to the tile in the map area   */
                struct bork_tile* tile = bork_map_tile_ptri(map, x, y, z);
                /*  If the tile is outside the map, or the tile is not
                    collidable, move on to the next one */
                if(!tile || tile->type < 2) continue;
                /*  Otherwise test collisions against this tile's associated
                    triangles in the area model */
                vec3 tile_push;
                int c = pg_model_collide_sphere_sub(model, tile_push,
                            pos, r, 1, tile->model_tri_idx, tile->num_tris);
                if(c >= 0) return 1;
            }
        }
    }
    return 0;
}

int bork_map_check_vis(struct bork_map* map, vec3 const start, vec3 const end)
{
    vec3 full_vec, part_vec, curr_point;
    vec3_sub(full_vec, end, start);
    float full_dist = vec3_len(full_vec);
    float part_dist = 0.25;
    float curr_dist = 0;
    vec3_set_len(part_vec, full_vec, part_dist);
    vec3_dup(curr_point, start);
    while(curr_dist <= full_dist) {
        if(bork_map_check_sphere(map, curr_point, 0.25)) return 0;
        if(curr_dist + part_dist > full_dist) vec3_dup(curr_point, end);
        else vec3_add(curr_point, curr_point, part_vec);
        curr_dist += part_dist;
    }
    return 1;
}

void bork_map_build_plr_dist(struct bork_map* map)
{
    memset(map->plr_dist, 0, sizeof(map->plr_dist));
    int plr_x = floor(map->plr->pos[0] / 2);
    int plr_y = floor(map->plr->pos[1] / 2);
    int plr_z = floor(map->plr->pos[2] / 2);
    /*  Start the queue with the player's current tile  */
    struct bork_tile* opp = bork_map_tile_ptri(map, plr_x, plr_y, plr_z + 1);
    struct bork_tile* tile = bork_map_tile_ptri(map, plr_x, plr_y, plr_z);
    map->plr_dist[plr_x][plr_y][plr_z] = 16;
    uint8_t queue[128][3] = { { plr_x, plr_y, plr_z } };
    int queue_idx[2] = { 0, 1 };
    while(queue_idx[0] != queue_idx[1]) {
        int x, y, z;
        x = queue[queue_idx[0]][0];
        y = queue[queue_idx[0]][1];
        z = queue[queue_idx[0]][2];
        int d = map->plr_dist[x][y][z];
        if(d > 1) {
            tile = bork_map_tile_ptri(map, x, y, z);
            printf("%s\n", bork_tile_detail(tile->type)->name);
            int i;
            for(i = 0; i < 4; ++i) {
                if(!(tile->travel_flags & (1 << i))) continue;
                int dx = x + PG_DIR_IDX[i][0];
                int dy = y + PG_DIR_IDX[i][1];
                int dz = z;
                opp = bork_map_tile_ptri(map, dx, dy, dz);
                if(!opp) continue;
                if(!map->plr_dist[dx][dy][z]) {
                    if(!(opp->travel_flags & (1 << 6))) continue;
                    if(opp->travel_flags & (1 << 7)) {
                        if(map->plr_dist[dx][dy][dz - 1]) continue;
                        map->plr_dist[dx][dy][dz] = d - 1;
                        --dz;
                    } else if(opp->travel_flags & (1 << 5)) {
                        map->plr_dist[dx][dy][dz + 1] = d - 1;
                    } else if(opp->type == BORK_TILE_RAMP_TOP) {
                        if(map->plr_dist[dx][dy][dz + 1]) {
                            map->plr_dist[dx][dy][dz] = map->plr_dist[dx][dy][dz + 1];
                            continue;
                        } else {
                            map->plr_dist[dx][dy][dz] = d - 1;
                            ++dz;
                        }
                    }
                    queue[queue_idx[1]][0] = dx;
                    queue[queue_idx[1]][1] = dy;
                    queue[queue_idx[1]][2] = dz;
                    map->plr_dist[dx][dy][dz] = d - 1;
                    queue_idx[1] = (queue_idx[1] + 1) % 128;
                }
            }
        }
        queue_idx[0] = (queue_idx[0] + 1) % 128;
    }
}

static int tile_travel(struct bork_tile* tile, int i)
{
    const struct bork_tile_detail* tile_d = bork_tile_detail(tile->type);
    if(tile->type == BORK_TILE_RAMP_BOTTOM || tile->type == BORK_TILE_RAMP_TOP) {
        if((tile->orientation & (1 << i))
        || (tile->orientation & (1 << PG_DIR_OPPOSITE[i]))) return 1;
        else return 0;
    } else {
        return ((tile_d->face_flags[i] & BORK_FACE_TRAVEL)
            || ((tile_d->face_flags[i] & BORK_FACE_TRAVEL_ORIENT)
                && (tile->orientation & (1 << i)))
            || ((tile_d->face_flags[i] & BORK_FACE_TRAVEL_ORIENT_OPP)
                && !(tile->orientation & (1 << i))));
    }
}

void bork_map_calc_travel(struct bork_map* map)
{
    struct bork_tile* tile;
    struct bork_tile* opp;
    const struct bork_tile_detail* tile_d;
    const struct bork_tile_detail* opp_d;
    int x, y, z, i;
    for(x = 0; x < 32; ++x)
    for(y = 0; y < 32; ++y)
    for(z = 1; z < 32; ++z) {
        tile = bork_map_tile_ptri(map, x, y, z);
        tile_d = bork_tile_detail(tile->type);
        tile->travel_flags = 0;
        if(tile_d->tile_flags & BORK_TILE_TRAVEL_DROP) {
            tile->travel_flags |= (1 << 6) | (1 << 5);
        } else if(tile->type == BORK_TILE_RAMP_TOP) tile->travel_flags |= (1 << 6);
        if(z > 0) {
            opp = bork_map_tile_ptri(map, x, y, z - 1);
            opp_d = bork_tile_detail(opp->type);
            if(opp_d->tile_flags & BORK_TILE_TRAVEL_DROP) {
                tile->travel_flags |= (1 << 7) | (1 << 6);
            }
            if(opp_d->tile_flags & BORK_TILE_WALK_ABOVE) tile->travel_flags |= (1 << 6);
        }
        for(i = 0; i < 4; ++i) {
            int dx = x + PG_DIR_IDX[i][0];
            int dy = y + PG_DIR_IDX[i][1];
            int dz = z + PG_DIR_IDX[i][2];
            opp = bork_map_tile_ptri(map, dx, dy, dz);
            if(!opp) continue;
            opp_d = bork_tile_detail(opp->type);
            int opp_travel = tile_travel(opp, PG_DIR_OPPOSITE[i]);
            if(opp->type != BORK_TILE_RAMP_TOP
            && !(opp_d->tile_flags & BORK_TILE_TRAVEL_DROP)) {
                opp = bork_map_tile_ptri(map, dx, dy, dz - 1);
                opp_d = bork_tile_detail(opp->type);
                if(!(opp_d->tile_flags & BORK_TILE_WALK_ABOVE)) continue;
            }
            tile->travel_flags |= ((tile_travel(tile, i) && opp_travel) << i);
        }
    }
}

/*  Model generation code   */

static void bork_map_generate_model(struct bork_map* map, struct pg_texture* env_atlas)
{
    pg_model_init(&map->model);
    map->model.components = PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV;
    struct bork_tile* tile;
    int x, y, z;
    for(x = 0; x < 32; ++x) {
        for(y = 0; y < 32; ++y) {
            for(z = 0; z < 32; ++z) {
                tile = &map->data[x][y][z];
                if(!tile || tile->type < 2) continue;
                struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
                tile->model_tri_idx = map->model.tris.len;
                tile->num_tris = detail->add_model(map, env_atlas, tile, x, y, z);
            }
        }
    }
    pg_model_precalc_ntb(&map->model);
}

/*  Generating geometry for individual tiles    */
/*  The BASIC tile geometry generation; just variations on a cube   */
static const vec3 vert_pos[6][4] = {
    { { 0.5, 0.5, -0.5 },
      { 0.5, 0.5, 0.5 },
      { -0.5, 0.5, -0.5 },
      { -0.5, 0.5, 0.5 } },
    { { -0.5, -0.5, -0.5 },
      { -0.5, -0.5, 0.5 },
      { 0.5, -0.5, -0.5 },
      { 0.5, -0.5, 0.5 } },
    { { 0.5, -0.5, -0.5 },
      { 0.5, -0.5, 0.5 },
      { 0.5, 0.5, -0.5 },
      { 0.5, 0.5, 0.5 } },
    { { -0.5, 0.5, -0.5 },
      { -0.5, 0.5, 0.5 },
      { -0.5, -0.5, -0.5 },
      { -0.5, -0.5, 0.5 } },
    { { -0.5, -0.5, 0.5 },
      { -0.5, 0.5, 0.5 },
      { 0.5, -0.5, 0.5 },
      { 0.5, 0.5, 0.5 } },
    { { -0.5, -0.5, -0.5 },
      { 0.5, -0.5, -0.5 },
      { -0.5, 0.5, -0.5 },
      { 0.5, 0.5, -0.5 } } };

static int tile_face_basic(struct bork_map* map, struct pg_texture* env_atlas,
                           struct bork_tile* tile, int x, int y, int z,
                           enum pg_direction dir)
{
    /*  Get the details for this face   */
    struct bork_tile_detail* detail = &BORK_TILE_DETAILS[tile->type];
    uint32_t tile_flags = detail->tile_flags;
    uint32_t face_flags = detail->face_flags[dir];
    if(!(face_flags & BORK_FACE_HAS_SURFACE)) return 0; /*  Tile has no face here   */
    /*  Get details for the opposing face   */
    int opp[3] = { x + PG_DIR_VEC[dir][0], y + PG_DIR_VEC[dir][1], z + PG_DIR_VEC[dir][2] };
    struct bork_tile* opp_tile = bork_map_tile_ptri(map, opp[0], opp[1], opp[2]);
    struct bork_tile_detail* opp_detail = opp_tile ?
        &BORK_TILE_DETAILS[opp_tile->type] : &BORK_TILE_DETAILS[0];
    uint32_t opp_flags = opp_detail->face_flags[PG_DIR_OPPOSITE[dir]];
    /*  Decide if this face of the tile needs to be generated   */
    if(opp_flags & BORK_FACE_HAS_SURFACE && !(opp_flags & BORK_FACE_SEETHRU_SURFACE)
    && !(face_flags & BORK_FACE_FORCE_SURFACE) && !(face_flags & BORK_FACE_FLUSH_SURFACE)) {
        return 0;
    } else if(face_flags & BORK_FACE_NO_SELF_OPPOSITE && opp_tile->type == tile->type) {
        return 0;
    } else if((tile_flags & BORK_TILE_FACE_ORIENTED) && dir < 4 && !(tile->orientation & (1 << dir))) {
        return 0;
    }
    struct pg_model* model = &map->model;
    /*  Calculate the base info for this face   */
    int num_tris = 0;
    vec3 inset_dir;
    vec3_scale(inset_dir, PG_DIR_VEC[dir], detail->face_inset[dir]);
    vec4 tex_frame;
    pg_texture_get_frame(env_atlas, detail->tex_tile[dir], tex_frame);
    unsigned vert_idx = model->v_count;
    /*  Generate the geometry   */
    struct pg_vertex_full new_vert = { .components =
        PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV };
    int i;
    if(!(face_flags & BORK_FACE_NO_FRONTFACE)) {
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == 0.5 && (face_flags & BORK_FACE_HALF_BOTTOM)) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] += (16.0f / 512.0f);
            } else if(new_vert.pos[2] == -0.5 && (face_flags & BORK_FACE_HALF_TOP)) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] -= (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 0, vert_idx + 2);
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 2, vert_idx + 3);
        num_tris += 2;
    }
    if(face_flags & BORK_FACE_HAS_BACKFACE) {
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == 0.5 && (face_flags & BORK_FACE_HALF_BOTTOM)) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] += (16.0f / 512.0f);
            } else if(new_vert.pos[2] == -0.5 && (face_flags & BORK_FACE_HALF_TOP)) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] -= (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_sub(new_vert.pos, new_vert.pos, inset_dir);
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 4, vert_idx + 5, vert_idx + 6);
        pg_model_add_triangle(model, vert_idx + 6, vert_idx + 5, vert_idx + 7);
        num_tris += 2;
    }
    return num_tris;
}

static int tile_model_basic(struct bork_map* map, struct pg_texture* env_atlas,
                            struct bork_tile* tile, int x, int y, int z)
{
    int tri_count = 0;
    int s;
    for(s = 0; s < 6; ++s) {
        tri_count += tile_face_basic(map, env_atlas, tile, x, y, z, s);
    }
    return tri_count;
}

static int tile_face_duct(struct bork_map* map, struct pg_texture* env_atlas,
                          struct bork_tile* tile, int x, int y, int z,
                          enum pg_direction dir)
{
    /*  Get details for the opposing face   */
    int opp[3] = { x + PG_DIR_VEC[dir][0], y + PG_DIR_VEC[dir][1], z + PG_DIR_VEC[dir][2] };
    struct bork_tile* opp_tile = bork_map_tile_ptri(map, opp[0], opp[1], opp[2]);
    struct bork_tile_detail* opp_detail = opp_tile ?
        &BORK_TILE_DETAILS[opp_tile->type] : &BORK_TILE_DETAILS[0];
    uint32_t opp_flags = opp_detail->face_flags[PG_DIR_OPPOSITE[dir]];
    /*  Decide if this face of the tile needs to be generated   */
    if(opp_tile->type == BORK_TILE_DUCT) return 0;
    vec4 tex_frame;
    struct pg_model* model = &map->model;
    unsigned vert_idx = model->v_count;
    struct pg_vertex_full new_vert = { .components =
        PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV };
    int i;
    if(dir == PG_TOP || dir == PG_BOTTOM) {
        pg_texture_get_frame(env_atlas, 21, tex_frame);
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == 0.5) new_vert.pos[2] = 0.0f;
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == 0.5) new_vert.pos[2] = 0.0f;
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 0, vert_idx + 1, vert_idx + 2);
        pg_model_add_triangle(model, vert_idx + 2, vert_idx + 1, vert_idx + 3);
        pg_model_add_triangle(model, vert_idx + 5, vert_idx + 4, vert_idx + 6);
        pg_model_add_triangle(model, vert_idx + 5, vert_idx + 6, vert_idx + 7);
        if(dir == PG_TOP && !(opp_flags & BORK_FACE_HAS_SURFACE))
        {
            pg_texture_get_frame(env_atlas, 3, tex_frame);
            for(i = 0; i < 4; ++i) {
                vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                         tex_frame[(1 - (i % 2)) * 2 + 1]);
                vec3_dup(new_vert.pos, vert_pos[dir][i]);
                vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
                vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
                vec3_scale(new_vert.pos, new_vert.pos, 2);
                pg_model_add_vertex(model, &new_vert);
            }
            pg_model_add_triangle(model, vert_idx + 9, vert_idx + 8, vert_idx + 10);
            pg_model_add_triangle(model, vert_idx + 9, vert_idx + 10, vert_idx + 11);
            return 6;
        }
        return 4;
    } else if(!(tile->orientation & (1 << dir))) {
        /*  Make the duct wall  */
        pg_texture_get_frame(env_atlas, 21, tex_frame);
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == 0.5) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] += (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == 0.5) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] += (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 0, vert_idx + 1, vert_idx + 2);
        pg_model_add_triangle(model, vert_idx + 2, vert_idx + 1, vert_idx + 3);
        pg_model_add_triangle(model, vert_idx + 5, vert_idx + 4, vert_idx + 6);
        pg_model_add_triangle(model, vert_idx + 5, vert_idx + 6, vert_idx + 7);
        return 4;
    } else if(!(opp_flags & BORK_FACE_HAS_SURFACE) || (opp_flags & BORK_FACE_SEETHRU_SURFACE)) {
        /*  Make the "outward" faces ie. regular walls  */
        pg_texture_get_frame(env_atlas, 19, tex_frame);
        for(i = 0; i < 4; ++i) {
            vec2_set(new_vert.uv, tex_frame[(1 - (i < 2)) * 2],
                     tex_frame[(1 - (i % 2)) * 2 + 1]);
            vec3_dup(new_vert.pos, vert_pos[dir][i]);
            if(new_vert.pos[2] == -0.5) {
                new_vert.pos[2] = 0.0f;
                new_vert.uv[1] -= (16.0f / 512.0f);
            }
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
            vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
            vec3_scale(new_vert.pos, new_vert.pos, 2);
            pg_model_add_vertex(model, &new_vert);
        }
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 0, vert_idx + 2);
        pg_model_add_triangle(model, vert_idx + 1, vert_idx + 2, vert_idx + 3);
        return 2;
    }
    return 0;
}

static int tile_model_duct(struct bork_map* map, struct pg_texture* env_atlas,
                           struct bork_tile* tile, int x, int y, int z)
{
    int tri_count = 0;
    tri_count += tile_face_duct(map, env_atlas, tile, x, y, z, PG_LEFT);
    tri_count += tile_face_duct(map, env_atlas, tile, x, y, z, PG_RIGHT);
    tri_count += tile_face_duct(map, env_atlas, tile, x, y, z, PG_FRONT);
    tri_count += tile_face_duct(map, env_atlas, tile, x, y, z, PG_BACK);
    tri_count += tile_face_duct(map, env_atlas, tile, x, y, z, PG_TOP);
    tri_count += tile_face_duct(map, env_atlas, tile, x, y, z, PG_BOTTOM);
    return tri_count;
}

static int tile_model_ramp(struct bork_map* map, struct pg_texture* env_atlas,
                           struct bork_tile* tile, int x, int y, int z)
{
    struct pg_model* model = &map->model;
    struct pg_vertex_full new_vert = { .components =
        PG_MODEL_COMPONENT_POSITION | PG_MODEL_COMPONENT_UV };
    vec4 tex_frame;
    pg_texture_get_frame(env_atlas, 6, tex_frame);
    unsigned vert_idx = model->v_count;
    int i;
    for(i = 0; i < 8; ++i) {
        int vi = i % 4;
        vec3_dup(new_vert.pos, vert_pos[5][vi]);
        vec2_set(new_vert.uv, tex_frame[(vi < 2) * 2],
                 tex_frame[(1 - (vi % 2)) * 2 + 1]);
        if(tile->type == BORK_TILE_RAMP_TOP) new_vert.pos[2] += 0.5;
        vec3_add(new_vert.pos, new_vert.pos, (vec3){ 0.5, 0.5, 0.5 });
        vec3_add(new_vert.pos, new_vert.pos, (vec3){ x, y, z });
        vec3_scale(new_vert.pos, new_vert.pos, 2);
        pg_model_add_vertex(model, &new_vert);
    }
    if(tile->orientation & (1 << PG_FRONT)) {
        model->pos.data[vert_idx + 2].v[2] += 1.0f;
        model->pos.data[vert_idx + 3].v[2] += 1.0f;
        model->pos.data[vert_idx + 6].v[2] += 1.0f;
        model->pos.data[vert_idx + 7].v[2] += 1.0f;
    } else if(tile->orientation & (1 << PG_BACK)) {
        model->pos.data[vert_idx + 0].v[2] += 1.0f;
        model->pos.data[vert_idx + 1].v[2] += 1.0f;
        model->pos.data[vert_idx + 4].v[2] += 1.0f;
        model->pos.data[vert_idx + 5].v[2] += 1.0f;
    } else if(tile->orientation & (1 << PG_LEFT)) {
        model->pos.data[vert_idx + 1].v[2] += 1.0f;
        model->pos.data[vert_idx + 3].v[2] += 1.0f;
        model->pos.data[vert_idx + 5].v[2] += 1.0f;
        model->pos.data[vert_idx + 7].v[2] += 1.0f;
    } else if(tile->orientation & (1 << PG_RIGHT)) {
        model->pos.data[vert_idx + 0].v[2] += 1.0f;
        model->pos.data[vert_idx + 2].v[2] += 1.0f;
        model->pos.data[vert_idx + 4].v[2] += 1.0f;
        model->pos.data[vert_idx + 6].v[2] += 1.0f;
    }
    pg_model_add_triangle(model, vert_idx + 0, vert_idx + 1, vert_idx + 2);
    pg_model_add_triangle(model, vert_idx + 2, vert_idx + 1, vert_idx + 3);
    pg_model_add_triangle(model, vert_idx + 5, vert_idx + 4, vert_idx + 6);
    pg_model_add_triangle(model, vert_idx + 5, vert_idx + 6, vert_idx + 7);
    return 4;
}

int bork_map_load_editor_map(struct bork_map* map, char* filename)
{
    struct bork_editor_map ed_map;
    int loaded = bork_editor_load_map(&ed_map, filename);
    if(!loaded) return 0;
    else bork_editor_complete_map(map, &ed_map);
    return 1;
}

void bork_map_create_fire(struct bork_map* map, vec3 pos, int lifetime)
{
    struct bork_fire new_fire = {
        .pos = { pos[0], pos[1], pos[2] },
        .lifetime = lifetime
    };
    ARR_PUSH(map->fires, new_fire);
}

void bork_map_add_enemy(struct bork_map* map, bork_entity_t ent_id)
{
    struct bork_entity* ent = bork_entity_get(ent_id);
    if(!ent) return;
    int x, y, z;
    x = (int)ent->pos[0] / 16;
    y = (int)ent->pos[1] / 16;
    z = (int)ent->pos[2] / 16;
    ARR_PUSH(map->enemies[x][y][z], ent_id);
    printf("Adding %d %d %d\n", x, y, z);
}

void bork_map_add_item(struct bork_map* map, bork_entity_t ent_id)
{
    struct bork_entity* ent = bork_entity_get(ent_id);
    if(!ent) return;
    int x, y, z;
    x = (int)ent->pos[0] / 16;
    y = (int)ent->pos[1] / 16;
    z = (int)ent->pos[2] / 16;
    ARR_PUSH(map->items[x][y][z], ent_id);
}

void bork_map_query_enemies(struct bork_map* map, bork_entity_arr_t* arr,
                            vec3 start, vec3 end)
{
    int start_i[3];
    int end_i[3];
    start_i[0] = CLAMP((int)start[0] / 16, 0, 3);
    start_i[1] = CLAMP((int)start[1] / 16, 0, 3);
    start_i[2] = CLAMP((int)start[2] / 16, 0, 3);
    end_i[0] = CLAMP((int)end[0] / 16, 0, 3);
    end_i[1] = CLAMP((int)end[1] / 16, 0, 3);
    end_i[2] = CLAMP((int)end[2] / 16, 0, 3);
    int x = start_i[0], y = start_i[1], z = start_i[2];
    do {
        do {
            do {
                int i;
                bork_entity_t ent_id;
                struct bork_entity* ent;
                ARR_FOREACH(map->enemies[x][y][z], ent_id, i) {
                    ent = bork_entity_get(ent_id);
                    if(!ent) continue;
                    //printf("%d\n", ent_id);
                    if(ent->pos[0] >= start[0] && ent->pos[0] <= end[0]
                    && ent->pos[1] >= start[1] && ent->pos[1] <= end[1]
                    && ent->pos[2] >= start[2] && ent->pos[2] <= end[2]) {
                        ARR_PUSH(*arr, ent_id);
                    }
                }
            } while(x++ < end_i[0]);
            x = start_i[0];
        } while(y++ < end_i[1]);
        y = start_i[1];
    } while(z++ < end_i[2]);
}

void bork_map_query_items(struct bork_map* map, bork_entity_arr_t* arr,
                          vec3 start, vec3 end)
{
    int start_i[3];
    int end_i[3];
    start_i[0] = CLAMP((int)start[0] / 16, 0, 3);
    start_i[1] = CLAMP((int)start[1] / 16, 0, 3);
    start_i[2] = CLAMP((int)start[2] / 16, 0, 3);
    end_i[0] = CLAMP((int)end[0] / 16, 0, 3);
    end_i[1] = CLAMP((int)end[1] / 16, 0, 3);
    end_i[2] = CLAMP((int)end[2] / 16, 0, 3);
    int x = start_i[0], y = start_i[1], z = start_i[2];
    do {
        do {
            do {
                int i;
                bork_entity_t ent_id;
                struct bork_entity* ent;
                ARR_FOREACH(map->items[x][y][z], ent_id, i) {
                    ent = bork_entity_get(ent_id);
                    if(!ent) continue;
                    if(ent->pos[0] >= start[0] && ent->pos[0] <= end[0]
                    && ent->pos[1] >= start[1] && ent->pos[1] <= end[1]
                    && ent->pos[2] >= start[2] && ent->pos[2] <= end[2]) {
                        ARR_PUSH(*arr, ent_id);
                    }
                }
            } while(x++ < end_i[0]);
            x = start_i[0];
        } while(y++ < end_i[1]);
        y = start_i[1];
    } while(z++ < end_i[2]);
}


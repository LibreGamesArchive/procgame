#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "procgl/wave_defs.h"
#include "bork.h"
#include "map_area.h"

enum bork_menu_option {
    BORK_MENU_NEW_GAME,
    BORK_MENU_CONTINUE,
    BORK_MENU_OPTIONS,
    BORK_MENU_CREDITS,
    BORK_MENU_EXIT,
    BORK_MENU_COUNT
};

const char* BORK_MENU_STRING[BORK_MENU_COUNT] = {
    "NEW GAME",
    "CONTINUE",
    "OPTIONS",
    "CREDITS",
    "EXIT" };

struct bork_menu_data {
    struct bork_game_core* core;
    const uint8_t* kb_state;
    vec2 mouse_motion;
    int current_selection;
    float option_x[BORK_MENU_COUNT];
};

static void bork_menu_tick(struct pg_game_state* state);
static void bork_menu_draw(struct pg_game_state* state);

static void bork_menu_generate_map(struct bork_menu_data* d, int w, int h, int l);

void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 30, 3);
    struct bork_menu_data* d = malloc(sizeof(*d));
    d->current_selection = 0;
    d->core = core;
    d->kb_state = SDL_GetKeyboardState(NULL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    vec2_set(d->mouse_motion, 0, 0);
    /*  Assign all the pointers, and it's finished  */
    state->data = d;
    state->update = NULL;
    state->tick = bork_menu_tick;
    state->draw = bork_menu_draw;
    state->deinit = free;
}

static void bork_menu_tick(struct pg_game_state* state)
{
    struct bork_menu_data* d = state->data;
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) state->tick = NULL;
        else if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.scancode) {
            case SDL_SCANCODE_DOWN:
                d->current_selection = MOD(d->current_selection + 1, BORK_MENU_COUNT);
                break;
            case SDL_SCANCODE_UP:
                d->current_selection = MOD(d->current_selection - 1, BORK_MENU_COUNT);
                break;
            case SDL_SCANCODE_RETURN:
                if(d->current_selection == BORK_MENU_NEW_GAME)
                    bork_play_start(state, d->core);
                break;
            default: break;
            }
        }
    }
}

static void bork_menu_draw(struct pg_game_state* state)
{
    struct bork_menu_data* d = state->data;
    /*  Overlay */
    vec2 title_size = {
        (float)d->core->screen_size[1] * 0.0625,
        (float)d->core->screen_size[1] * 0.25 };
    vec2 title_coord = {
        (float)d->core->screen_size[0] * 0.45 - (title_size[0] * 4 + title_size[0] * 3),
        (float)d->core->screen_size[1] * 0.375 };
    vec2 option_font_size = {
        (float)d->core->screen_size[1] * 0.02,
        (float)d->core->screen_size[1] * 0.1 };
    vec2 option_block_coord = {
        (float)d->core->screen_size[0] * 0.55,
        (float)d->core->screen_size[1] * 0.15 };
    float option_selected_offset = (float)d->core->screen_size[1] * 0.04;
    float option_block_height = (float)d->core->screen_size[1] * 0.7;
    float option_step = option_block_height / (float)BORK_MENU_COUNT;
    pg_screen_dst();
    pg_shader_begin(&d->core->shader_text, NULL);
    char str[128];
    snprintf(str, 10, "FPS: %d", d->current_selection);
    pg_shader_text_write(&d->core->shader_text, str,
        (vec2){ 0, 0 }, (vec2){ 8, 8 }, 0.25);
    pg_shader_text_write(&d->core->shader_text, "BORK",
        title_coord, title_size, 0.75);
    int i;
    for(i = 0; i < BORK_MENU_COUNT; ++i) {
        pg_shader_text_write(&d->core->shader_text, BORK_MENU_STRING[i],
            (vec2){ option_block_coord[0] + d->option_x[i] +
                        ((d->current_selection == i) * option_selected_offset),
                    option_block_coord[1] },
            option_font_size, 0.75);
        option_block_coord[1] += option_step;
    }
}


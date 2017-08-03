#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "entity.h"
#include "map_area.h"

enum bork_menu_option {
    BORK_MENU_NEW_GAME,
    BORK_MENU_CONTINUE,
    BORK_MENU_OPTIONS,
    BORK_MENU_CREDITS,
    BORK_MENU_EDITOR,
    BORK_MENU_EXIT,
    BORK_MENU_COUNT
};

const char* BORK_MENU_STRING[BORK_MENU_COUNT] = {
    "NEW GAME",
    "CONTINUE",
    "OPTIONS",
    "CREDITS",
    "EDITOR",
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

void bork_menu_start(struct pg_game_state* state, struct bork_game_core* core)
{
    /*  Set up the game state, 60 ticks per second, keyboard input   */
    pg_game_state_init(state, pg_time(), 30, 3);
    struct bork_menu_data* d = malloc(sizeof(*d));
    *d = (struct bork_menu_data) {
        .core = core,
        .kb_state = SDL_GetKeyboardState(NULL) };
    //SDL_SetRelativeMouseMode(SDL_TRUE);
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
        if(e.type == SDL_QUIT) state->running = 0;
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
                else if(d->current_selection == BORK_MENU_EDITOR)
                    bork_editor_start(state, d->core);
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
    pg_screen_dst();
    struct pg_shader* shader = &d->core->shader_text;
    bork_draw_backdrop(d->core, (vec4){ 1, 1, 1, 1 },
                       (float)state->ticks / (float)state->tick_rate);
    /*  Using text in screen space [0,1]    */
    pg_shader_text_resolution(shader, (vec2){ 1, 1 });
    /*  Ratio to un-distort text in non-1:1 windows   */
    float font_ratio = d->core->font.frame_aspect_ratio / d->core->aspect_ratio;
    pg_shader_begin(shader, NULL);
    pg_shader_text_transform(shader,
        (vec2){ 0.6, 0.15 }, (vec2){ font_ratio * 0.05, 0.09 });
    struct pg_shader_text menutext = { .use_blocks = BORK_MENU_COUNT + 1 };
    int i;
    for(i = 0; i < BORK_MENU_COUNT; ++i) {
        strncpy(menutext.block[i], BORK_MENU_STRING[i], strlen(BORK_MENU_STRING[i]));
        vec4_set(menutext.block_style[i], (d->current_selection == i) * 2, i * 1.4, 1, 1.2);
        vec4_set(menutext.block_color[i], 1, 1, 1, 1);
    }
    pg_shader_text_write(shader, &menutext);
    /*  Title is drawn separately   */
    float title_offset = 0.3 - font_ratio * 0.2 * 1.2 * 2;
    pg_shader_text_transform(shader, (vec2){ title_offset, 0.3 }, (vec2){ 0.2 * font_ratio, 0.4 });
    memset(menutext.block[0], 0, 64);
    strncpy(menutext.block[0], "BORK", 4);
    vec4_set(menutext.block_style[0], 0, 0, 1, 1.2);
    vec4_set(menutext.block_color[0], 1, 1, 1, 0.5);
    menutext.use_blocks = 1;
    pg_shader_text_write(shader, &menutext);
    pg_shader_text_resolution(shader, d->core->screen_size);
    bork_draw_fps(d->core);
}


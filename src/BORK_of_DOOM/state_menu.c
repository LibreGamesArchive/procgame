#include <stdio.h>
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
    pg_poll_input();
    if(pg_user_exit()) state->running = 0;
    int stick_ctrl = 0;
    if(pg_check_gamepad(PG_LEFT_STICK, PG_CONTROL_HIT)) {
        vec2 stick;
        pg_gamepad_stick(0, stick);
        if(fabsf(stick[1]) > 0.6) stick_ctrl = SGN(stick[1]);
    }
    if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HIT) || stick_ctrl == 1) {
        d->current_selection = MOD(d->current_selection + 1, BORK_MENU_COUNT);
        pg_audio_play(&d->core->menu_sound, 0.2);
    }
    if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HIT) || stick_ctrl == -1) {
        d->current_selection = MOD(d->current_selection - 1, BORK_MENU_COUNT);
        pg_audio_play(&d->core->menu_sound, 0.2);
    }
    if(pg_check_input(SDL_SCANCODE_RETURN, PG_CONTROL_HIT)
    || pg_check_gamepad(SDL_CONTROLLER_BUTTON_A, PG_CONTROL_HIT)) {
        if(d->current_selection == BORK_MENU_NEW_GAME)
            bork_play_start(state, d->core);
        else if(d->current_selection == BORK_MENU_EDITOR)
            bork_editor_start(state, d->core);
    }
    pg_flush_input();
}

static struct pg_shader_text main_menu_text = {
    .use_blocks = 9,
    .block = {
        "NEW GAME",
        "CONTINUE",
        "OPTIONS",
        "CREDITS",
        "EDITOR",
        "EXIT",
        "BORK", "OF", "DOOM!",
    },
    .block_style = {
        {0.3, -0.5, 0.05, 1.2},
        {0.3, -0.3, 0.05, 1.2},
        {0.3, -0.1, 0.05, 1.2},
        {0.3, 0.1, 0.05, 1.2},
        {0.3, 0.3, 0.05, 1.2},
        {0.3, 0.5, 0.05, 1.2},
        /*  BORK OF DOOOOOM */
        { -0.95, -0.1, 0.2, 1.1 },
        { -0.9, 0.16, 0.05, 1.1 },
        { -0.7, 0.12, 0.12, 1.1 },
    },
    .block_color = {
        { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 },
        { 1, 1, 1, 1 }, { 1, 1, 1, 1 }, { 1, 1, 1, 1 },
        { 1, 1, 1, 0.7 }, { 1, 1, 1, 0.6 }, { 1, 0, 0, 1 },
    },
};

static void bork_menu_draw(struct pg_game_state* state)
{
    struct bork_menu_data* d = state->data;
    /*  Overlay */
    pg_screen_dst();
    struct pg_shader* shader = &d->core->shader_text;
    bork_draw_backdrop(d->core, (vec4){ 2, 2, 2, 1 },
                       (float)state->ticks / (float)state->tick_rate);
    bork_draw_linear_vignette(d->core, (vec4){ 0, 0, 0, 1 });
    pg_shader_begin(shader, NULL);
    pg_shader_text_ndc(shader, (vec2){d->core->aspect_ratio, 1});
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){});
    int i;
    for(i = 0; i < BORK_MENU_COUNT; ++i) {
        vec4_set(main_menu_text.block_style[i],
            (d->current_selection == i) * 0.15 + 0.1,
            i * 0.2 - 0.5, 0.075, 1.2);
        vec4_set(main_menu_text.block_color[i], 1, 1, 1, 1);
    }
    pg_shader_text_write(shader, &main_menu_text);
    /*  Title is drawn separately   */
    pg_shader_text_resolution(shader, d->core->screen_size);
    bork_draw_fps(d->core);
}


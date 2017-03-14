#include <stdlib.h>
#include "game_state.h"

void pg_game_state_init(struct pg_game_state* state, float start_time,
                        unsigned tick_rate, unsigned tick_max)
{
    state->ticks = 0;
    state->tick_rate = tick_rate;
    state->tick_max = tick_max;
    state->last_tick = start_time;
    state->tick_over = 0;
    state->time = start_time;
    state->data = NULL;
    state->tick = NULL;
    state->draw = NULL;
}

void pg_game_state_deinit(struct pg_game_state* state)
{
    if(state->deinit) state->deinit(state->data);
}

void pg_game_state_update(struct pg_game_state* state, float new_time)
{
    float time_elapsed = new_time - state->last_tick;
    float tick_time = 1.0f / state->tick_rate;
    float acc = 0;
    unsigned ticks_done = 0;
    while(acc + tick_time < time_elapsed && ticks_done < state->tick_max) {
        ++ticks_done;
        acc += tick_time;
        state->time = state->last_tick + acc;
        state->tick(state);
    }
    state->last_tick = state->last_tick + acc;
    state->tick_over = (time_elapsed - acc) / tick_time;
    state->time = new_time;
}

void pg_game_state_draw(struct pg_game_state* state)
{
    state->draw(state);
}
struct pg_game_state {
    unsigned ticks;     /*  Ticks done so far   */
    unsigned tick_rate; /*  Ticks per second    */
    unsigned tick_max;  /*  Max ticks per update    */
    float last_tick;    /*  Time of last tick   */
    float tick_over;    /*  Time since last tick (fraction of a tick)   */
    float time;         /*  Time in seconds */
    void* data;         /*  User data   */
    void (*tick)(struct pg_game_state*);
    void (*draw)(struct pg_game_state*);
    void (*deinit)(void*);
};

void pg_game_state_init(struct pg_game_state* state, float start_time,
                        unsigned tick_rate, unsigned tick_max);
void pg_game_state_deinit(struct pg_game_state* state);
void pg_game_state_update(struct pg_game_state* state, float new_time);
void pg_game_state_draw(struct pg_game_state* state);

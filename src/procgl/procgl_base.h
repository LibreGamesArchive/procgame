int pg_init(int w, int h, int fullscreen, const char* window_title);
void pg_deinit(void);
void pg_screen_size(int* w, int* h);
void pg_screen_swap(void);
void pg_screen_dst(void);

/*  Returns the time in seconds since the last call to this function, or 0
    the first time it is called. If dump is non-zero, the value will not be
    used toward framerate calculations and will not reset the delta time for
    the next call   */
/*  Returns framerate based on calls to pg_delta_time   */
void pg_calc_framerate(float new_time);
float pg_framerate(void);
/*  Just returns the current time in seconds    */
float pg_time(void);

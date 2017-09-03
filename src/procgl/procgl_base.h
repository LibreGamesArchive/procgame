enum pg_direction {
    PG_FRONT = 0,
    PG_Y_POS = PG_FRONT,
    PG_BACK = 1,
    PG_Y_NEG = PG_BACK,
    PG_LEFT = 2,
    PG_X_POS = PG_LEFT,
    PG_RIGHT = 3,
    PG_X_NEG = PG_RIGHT,
    PG_UP = 4,
    PG_Z_POS = PG_UP,
    PG_TOP = PG_UP,
    PG_DOWN = 5,
    PG_Z_NEG = PG_DOWN,
    PG_BOTTOM = PG_DOWN
};
static const int PG_DIR_OPPOSITE[6] = { 1, 0, 3, 2, 5, 4 };
static const vec3 PG_DIR_VEC[6] = {
    [PG_FRONT] = { 0, 1.0f, 0 },
    [PG_BACK] = { 0, -1.0f, 0 },
    [PG_LEFT] = { 1.0f, 0, 0 },
    [PG_RIGHT] = { -1.0f, 0, 0 },
    [PG_UP] = { 0, 0, 1.0f },
    [PG_DOWN] = { 0, 0, -1.0f } };
static const vec3 PG_DIR_TAN[6] = {
    [PG_FRONT] = { -1.0f, 0, 0 },
    [PG_BACK] = { 1.0f, 0, 0 },
    [PG_LEFT] = { 0, 1.0f, 0 },
    [PG_RIGHT] = { 0, -1.0f, 0 },
    [PG_UP] = { 1.0f, 0, 0 },
    [PG_DOWN] = { -1.0f, 0, 0 } };
static const vec3 PG_DIR_BITAN[6] = {
    [PG_FRONT] = { 0, 0, 1.0f },
    [PG_BACK] = { 0, 0, -1.0f },
    [PG_LEFT] = { 0, 0, 1.0f },
    [PG_RIGHT] = { 0, 0, -1.0f },
    [PG_UP] = { 0, 1.0f, 0 },
    [PG_DOWN] = { 0, -1.0f, 0 } };

int pg_init(int w, int h, int fullscreen, const char* window_title);
void pg_deinit(void);
void pg_screen_size(int* w, int* h);
void pg_screen_swap(void);
void pg_screen_dst(void);

/*  The possible control states */
#define PG_CONTROL_OFF          (1 << 0)
#define PG_CONTROL_RELEASED     (1 << 1)
#define PG_CONTROL_HIT          (1 << 2)
#define PG_CONTROL_HELD         (1 << 3)
/*  Special mouse control codes, mapped to unused SDL scancodes */
#define PG_LEFT_MOUSE           253
#define PG_RIGHT_MOUSE          254
#define PG_MIDDLE_MOUSE         255
/*  Input functions */
void pg_poll_input(void);
void pg_flush_input(void);
int pg_check_input(uint8_t ctrl, uint8_t state);
int pg_user_exit(void);
void pg_mouse_mode(int grab);
void pg_mouse_pos(vec2 out);
void pg_mouse_motion(vec2 out);

#define PG_LEFT_STICK       (SDL_CONTROLLER_BUTTON_MAX + 1)
#define PG_RIGHT_STICK      (SDL_CONTROLLER_BUTTON_MAX + 2)
#define PG_LEFT_TRIGGER     (SDL_CONTROLLER_BUTTON_MAX + 3)
#define PG_RIGHT_TRIGGER    (SDL_CONTROLLER_BUTTON_MAX + 4)
void pg_use_gamepad(int gpad_idx);
int pg_have_gamepad(void);
void pg_gamepad_config(float stick_dead_zone, float stick_threshold,
                       float trigger_dead_zone, float trigger_threshold);
int pg_check_gamepad(uint8_t ctrl, uint8_t state);
void pg_gamepad_stick(int side, vec2 out);
float pg_gamepad_trigger(int side);

/*  Returns the time in seconds since the last call to this function, or 0
    the first time it is called. If dump is non-zero, the value will not be
    used toward framerate calculations and will not reset the delta time for
    the next call   */
/*  Returns framerate based on calls to pg_delta_time   */
void pg_calc_framerate(float new_time);
float pg_framerate(void);
/*  Just returns the current time in seconds    */
float pg_time(void);

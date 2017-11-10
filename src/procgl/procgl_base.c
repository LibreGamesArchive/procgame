#include <stdio.h>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "ext/linmath.h"
#include "wave.h"
#include "audio.h"
#include "procgl_base.h"

static SDL_GLContext pg_context;
static SDL_Window* pg_window;
int screen_w, screen_h;
int render_w, render_h;

static SDL_GameController* pg_gamepad = NULL;
static SDL_JoystickID gamepad_id = -1;
static uint8_t gamepad_state[32] = {};
static uint8_t gamepad_changes[16] = {};
static uint8_t gamepad_changed = 0;
static vec2 gamepad_stick[2] = {};
static float gamepad_trigger[2] = {};
float pg_stick_dead_zone, pg_stick_threshold;
float pg_trigger_dead_zone, pg_trigger_threshold;

static uint8_t ctrl_state[256] = {};
static uint8_t ctrl_changes[16] = {};
static uint8_t ctrl_changed = 0;
static vec2 mouse_pos = {};
static vec2 mouse_motion = {};
static int mouse_relative = 0;
static int text_mode = 0;
static char text_input[16];
static char text_len;
static int user_exit = 0;

int pg_init(int w, int h, int fullscreen, const char* window_title)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetSwapInterval(0);
    SDL_DisplayMode display;
    SDL_GetDesktopDisplayMode(0, &display);
    screen_w = fullscreen ? display.w : w;
    screen_h = fullscreen ? display.h : h;
    render_w = w;
    render_h = h;
    user_exit = 0;
    pg_window = SDL_CreateWindow(window_title,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 screen_w, screen_h, SDL_WINDOW_OPENGL |
                                 (fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
    pg_context = SDL_GL_CreateContext(pg_window);
    SDL_GL_SetSwapInterval(1);
    glewExperimental = GL_TRUE;
    glewInit();
    glGetError();
    /*  Init audio system   */
    pg_init_audio();
    /*  Load gamepad mappings   */
    SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
    return 1;
}

void pg_deinit(void)
{
    SDL_GL_DeleteContext(pg_context);
    SDL_DestroyWindow(pg_window);
    SDL_Quit();
}

void pg_window_resize(int w, int h, int fullscreen)
{
    SDL_DisplayMode display;
    SDL_GetDesktopDisplayMode(0, &display);
    screen_w = fullscreen ? display.w : w;
    screen_h = fullscreen ? display.h : h;
    render_w = w;
    render_h = h;
    SDL_SetWindowSize(pg_window, screen_w, screen_h);
    SDL_SetWindowFullscreen(pg_window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
}

void pg_screen_size(int* w, int* h)
{
    if(w) *w = render_w;
    if(h) *h = render_h;
}

void pg_screen_swap(void)
{
    SDL_GL_SwapWindow(pg_window);
}

void pg_screen_dst(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screen_w, screen_h);
}

float pg_time(void)
{
    return (float)SDL_GetTicks() * 0.001;
}

static float last_time = 0;
static float framerate = 0;
static float framerate_last_update = 0;
static float framerate_update_interval = 0.5;
void pg_calc_framerate(float new_time)
{
    if(new_time >= framerate_last_update + framerate_update_interval) {
        framerate_last_update = new_time;
        framerate += 1.0f / (new_time - last_time);
        framerate /= 2;
    }
    last_time = new_time;
}

float pg_framerate(void)
{
    return framerate;
}

int pg_user_exit(void)
{
    return user_exit;
}

void pg_poll_input(void)
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT) user_exit = 1;
        else if(e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            int down = (e.type == SDL_MOUSEBUTTONDOWN);
            switch(e.button.button) {
            case SDL_BUTTON_LEFT:
                ctrl_state[PG_LEFT_MOUSE] =
                    down ? PG_CONTROL_HIT : PG_CONTROL_RELEASED;
                ctrl_changes[ctrl_changed++] = PG_LEFT_MOUSE;
                break;
            case SDL_BUTTON_RIGHT:
                ctrl_state[PG_RIGHT_MOUSE] =
                    down ? PG_CONTROL_HIT : PG_CONTROL_RELEASED;
                ctrl_changes[ctrl_changed++] = PG_RIGHT_MOUSE;
                break;
            case SDL_BUTTON_MIDDLE:
                ctrl_state[PG_MIDDLE_MOUSE] =
                    down ? PG_CONTROL_HIT : PG_CONTROL_RELEASED;
                ctrl_changes[ctrl_changed++] = PG_MIDDLE_MOUSE;
                break;
            }
        } else if(e.type == SDL_MOUSEWHEEL) {
            if(e.wheel.y == 1) {
                ctrl_state[PG_MOUSEWHEEL_UP] = PG_CONTROL_HIT;
                ctrl_changes[ctrl_changed++] = PG_MOUSEWHEEL_UP;
            } else if(e.wheel.y == -1) {
                ctrl_state[PG_MOUSEWHEEL_DOWN] = PG_CONTROL_HIT;
                ctrl_changes[ctrl_changed++] = PG_MOUSEWHEEL_DOWN;
            }
        } else if(e.type == SDL_TEXTINPUT && text_mode && text_len < 16) {
            int len = strnlen(e.text.text, 32);
            int buf_left = 16 - text_len;
            strncpy(text_input + text_len, e.text.text, buf_left);
            text_len = MIN(16, text_len + len);
        } else if(e.type == SDL_KEYDOWN) {
            if(ctrl_state[e.key.keysym.scancode] != PG_CONTROL_HELD) {
                ctrl_state[e.key.keysym.scancode] = PG_CONTROL_HIT;
                ctrl_changes[ctrl_changed++] = e.key.keysym.scancode;
            }
        } else if(e.type == SDL_KEYUP) {
            ctrl_state[e.key.keysym.scancode] = PG_CONTROL_RELEASED;
            ctrl_changes[ctrl_changed++] = e.key.keysym.scancode;
        } else if(e.type == SDL_CONTROLLERBUTTONDOWN && e.cbutton.which == gamepad_id) {
            if(gamepad_state[e.cbutton.button] != PG_CONTROL_HELD) {
                gamepad_state[e.cbutton.button] = PG_CONTROL_HIT;
                gamepad_changes[gamepad_changed++] = e.cbutton.button;
            }
        } else if(e.type == SDL_CONTROLLERBUTTONUP && e.cbutton.which == gamepad_id) {
            gamepad_state[e.cbutton.button] = PG_CONTROL_RELEASED;
            gamepad_changes[gamepad_changed++] = e.cbutton.button;
        } else if(e.type == SDL_CONTROLLERDEVICEREMOVED && e.cdevice.which == gamepad_id) {
            printf("Gamepad disconnected.\n");
            SDL_GameControllerClose(pg_gamepad);
            pg_gamepad = NULL;
        } else if(e.type == SDL_CONTROLLERDEVICEADDED && !pg_gamepad) {
            pg_use_gamepad(e.cdevice.which);
            printf("Gamepad connected.\n");
        }
    }
    int mx, my;
    if(mouse_relative) {
        SDL_GetRelativeMouseState(&mx, &my);
        vec2 motion = { (float)mx, (float)my };
        vec2_add(mouse_motion, mouse_motion, motion);
        vec2_add(mouse_pos, mouse_pos, motion);
        vec2_clamp(mouse_pos, mouse_pos, (vec2){}, (vec2){ render_w, render_h });
    } else {
        SDL_GetMouseState(&mx, &my);
        mx = ((float)mx / screen_w) * render_w;
        my = ((float)my / screen_h) * render_h;
        vec2_set(mouse_motion, mouse_pos[0] - mx, mouse_pos[1] - my);
        vec2_set(mouse_pos, mx, my);
    }
    if(pg_gamepad) {
        int16_t left[2], right[2], trigger[2];
        left[0] = SDL_GameControllerGetAxis(pg_gamepad, SDL_CONTROLLER_AXIS_LEFTX);
        left[1] = SDL_GameControllerGetAxis(pg_gamepad, SDL_CONTROLLER_AXIS_LEFTY);
        right[0] = SDL_GameControllerGetAxis(pg_gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
        right[1] = SDL_GameControllerGetAxis(pg_gamepad, SDL_CONTROLLER_AXIS_RIGHTY);
        trigger[0] = SDL_GameControllerGetAxis(pg_gamepad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        trigger[1] = SDL_GameControllerGetAxis(pg_gamepad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        vec2_set(gamepad_stick[0], (float)left[0] / INT16_MAX, (float)left[1] / INT16_MAX);
        vec2_set(gamepad_stick[1], (float)right[0] / INT16_MAX, (float)right[1] / INT16_MAX);
        gamepad_trigger[0] = trigger[0] / INT16_MAX;
        gamepad_trigger[1] = trigger[1] / INT16_MAX;
        if(fabsf(gamepad_stick[0][0]) < pg_stick_dead_zone) gamepad_stick[0][0] = 0;
        if(fabsf(gamepad_stick[0][1]) < pg_stick_dead_zone) gamepad_stick[0][1] = 0;
        if(fabsf(gamepad_stick[1][0]) < pg_stick_dead_zone) gamepad_stick[1][0] = 0;
        if(fabsf(gamepad_stick[1][1]) < pg_stick_dead_zone) gamepad_stick[1][1] = 0;
        if(gamepad_trigger[0] < pg_trigger_dead_zone) gamepad_trigger[0] = 0;
        if(gamepad_trigger[1] < pg_trigger_dead_zone) gamepad_trigger[1] = 0;
        /*  Also provide a button-like interface to the joysticks and triggers  */
        if(vec2_len(gamepad_stick[0]) > pg_stick_threshold) {
            if(gamepad_state[PG_LEFT_STICK] != PG_CONTROL_HELD) {
                gamepad_state[PG_LEFT_STICK] = PG_CONTROL_HIT;
                gamepad_changes[gamepad_changed++] = PG_LEFT_STICK;
            }
        } else if(gamepad_state[PG_LEFT_STICK] == PG_CONTROL_HELD) {
            gamepad_state[PG_LEFT_STICK] = PG_CONTROL_RELEASED;
            gamepad_changes[gamepad_changed++] = PG_LEFT_STICK;
        }
        if(vec2_len(gamepad_stick[1]) > pg_stick_threshold) {
            if(gamepad_state[PG_RIGHT_STICK] != PG_CONTROL_HELD) {
                gamepad_state[PG_RIGHT_STICK] = PG_CONTROL_HIT;
                gamepad_changes[gamepad_changed++] = PG_RIGHT_STICK;
            }
        } else if(gamepad_state[PG_RIGHT_STICK] == PG_CONTROL_HELD) {
            gamepad_state[PG_RIGHT_STICK] = PG_CONTROL_RELEASED;
            gamepad_changes[gamepad_changed++] = PG_RIGHT_STICK;
        }
        if(gamepad_trigger[0] > pg_trigger_threshold) {
            if(gamepad_state[PG_LEFT_TRIGGER] != PG_CONTROL_HELD) {
                gamepad_state[PG_LEFT_TRIGGER] = PG_CONTROL_HIT;
                gamepad_changes[gamepad_changed++] = PG_LEFT_TRIGGER;
            }
        } else if(gamepad_state[PG_LEFT_TRIGGER] == PG_CONTROL_HELD) {
            gamepad_state[PG_LEFT_TRIGGER] = PG_CONTROL_RELEASED;
            gamepad_changes[gamepad_changed++] = PG_LEFT_TRIGGER;
        }
        if(gamepad_trigger[1] > pg_trigger_threshold) {
            if(gamepad_state[PG_RIGHT_TRIGGER] != PG_CONTROL_HELD) {
                gamepad_state[PG_RIGHT_TRIGGER] = PG_CONTROL_HIT;
                gamepad_changes[gamepad_changed++] = PG_RIGHT_TRIGGER;
            }
        } else if(gamepad_state[PG_RIGHT_TRIGGER] == PG_CONTROL_HELD) {
            gamepad_state[PG_RIGHT_TRIGGER] = PG_CONTROL_RELEASED;
            gamepad_changes[gamepad_changed++] = PG_RIGHT_TRIGGER;
        }
    }
}

int pg_check_input(uint8_t ctrl, uint8_t event)
{
    if(ctrl_state[ctrl] & event) return 1;
    int changes = 0;
    int i;
    for(i = 0; i < ctrl_changed; ++i) {
        changes += (ctrl_changes[i] == ctrl);
        if(changes > 1) return 1;
    }
    return 0;
}

uint8_t pg_first_input(void)
{
    if(!ctrl_changed) return 0;
    int i = 0;
    while(i < ctrl_changed && ctrl_state[ctrl_changes[i]] == PG_CONTROL_RELEASED) ++i;
    if(i == ctrl_changed) return 0;
    return ctrl_changes[i];
}

int pg_check_keycode(int key, uint8_t event)
{
    uint8_t ctrl = SDL_GetScancodeFromKey(key);
    if(ctrl_state[ctrl] & event) return 1;
    int changes = 0;
    int i;
    for(i = 0; i < ctrl_changed; ++i) {
        changes += (ctrl_changes[i] == ctrl);
        if(changes > 1) return 1;
    }
    return 0;
}

void pg_flush_input(void)
{
    int i;
    for(i = 0; i < ctrl_changed; ++i) {
        uint8_t c = ctrl_changes[i];
        switch(ctrl_state[c]) {
        case 0: break;
        case PG_CONTROL_HIT:
            ctrl_state[c] = PG_CONTROL_HELD;
            break;
        case PG_CONTROL_HELD: case PG_CONTROL_RELEASED:
            ctrl_state[c] = PG_CONTROL_OFF;
            break;
        }
    }
    vec2_set(mouse_motion, 0, 0);
    ctrl_changed = 0;
    text_len = 0;
    if(!pg_gamepad) return;
    for(i = 0; i < gamepad_changed; ++i) {
        uint8_t c = gamepad_changes[i];
        switch(gamepad_state[c]) {
        case 0: break;
        case PG_CONTROL_HIT:
            gamepad_state[c] = PG_CONTROL_HELD;
            break;
        case PG_CONTROL_HELD: case PG_CONTROL_RELEASED:
            gamepad_state[c] = PG_CONTROL_OFF;
            break;
        }
    }
    gamepad_changed = 0;
}

int pg_copy_text_input(char* out, int n)
{
    int len = MIN(n, text_len);
    strncpy(out, text_input, len);
    out[len] = '\0';
    return len;
}

void pg_text_mode(int mode)
{
    text_mode = mode;
}

const char* pg_input_name(uint8_t ctrl)
{
    static const char* mouse_input_names[] = {
        "WHEEL UP", "WHEEL DOWN",
        "LEFT MOUSE", "RIGHT MOUSE", "MIDDLE MOUSE" };
    if(ctrl >= PG_MOUSEWHEEL_UP && ctrl <= PG_MIDDLE_MOUSE) {
        return mouse_input_names[ctrl - PG_MOUSEWHEEL_UP];
    } else {
        return SDL_GetKeyName(SDL_GetKeyFromScancode(ctrl));
    }
}
void pg_mouse_mode(int grab)
{
    if(grab) {
        SDL_SetRelativeMouseMode(SDL_ENABLE);
        SDL_GetRelativeMouseState(NULL, NULL);
        mouse_relative = 1;
    } else {
        SDL_SetRelativeMouseMode(SDL_DISABLE);
        mouse_relative = 0;
    }
}

void pg_mouse_pos(vec2 out)
{
    vec2_dup(out, mouse_pos);
}

void pg_mouse_motion(vec2 out)
{
    vec2_dup(out, mouse_motion);
}

void pg_use_gamepad(int gpad_idx)
{
    pg_gamepad = SDL_GameControllerOpen(gpad_idx);
    if(!pg_gamepad) {
        printf("procgame failed to open gamepad %d\n", gpad_idx);
        gamepad_id = -1;
    } else {
        gamepad_id = gpad_idx;
        SDL_Joystick* joy = SDL_GameControllerGetJoystick(pg_gamepad);
        gamepad_id = SDL_JoystickInstanceID(joy);
    }
}

int pg_have_gamepad(void)
{
    if(pg_gamepad) return 1;
    else return 0;
}

void pg_gamepad_config(float stick_dead_zone, float stick_threshold,
                       float trigger_dead_zone, float trigger_threshold)
{
    pg_stick_dead_zone = stick_dead_zone;
    pg_stick_threshold = stick_threshold;
    pg_trigger_dead_zone = trigger_dead_zone;
    pg_trigger_threshold = trigger_threshold;
}

int pg_check_gamepad(uint8_t ctrl, uint8_t state)
{
    if(gamepad_state[ctrl] & state) return 1;
    //printf("Button state for %d: %d, checking for %d\n", (int)ctrl, (int)gamepad_state[ctrl], (int)state);
    int changes = 0;
    int i;
    for(i = 0; i < gamepad_changed; ++i) {
        changes += (gamepad_changes[i] == ctrl);
        if(changes > 1) return 1;
    }
    return 0;
}

void pg_gamepad_stick(int side, vec2 out)
{
    side = side % 2;
    vec2_dup(out, gamepad_stick[side]);
}

float pg_gamepad_trigger(int side)
{
    return gamepad_trigger[side % 2];
}

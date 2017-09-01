#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "ext/linmath.h"
#include "wave.h"
#include "audio.h"
#include "procgl_base.h"

SDL_GLContext pg_context;
SDL_Window* pg_window;
int screen_w, screen_h;
int render_w, render_h;

static uint8_t ctrl_state[256];
static uint8_t ctrl_changes[16];
static uint8_t ctrl_changed;
static vec2 mouse_pos;
static vec2 mouse_motion;
static int mouse_relative;
static int user_exit;

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
    return 1;
}

void pg_deinit(void)
{
    SDL_GL_DeleteContext(pg_context);
    SDL_DestroyWindow(pg_window);
    SDL_Quit();
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
        } else if(e.type == SDL_KEYDOWN) {
            if(ctrl_state[e.key.keysym.scancode] != PG_CONTROL_HELD) {
                ctrl_state[e.key.keysym.scancode] = PG_CONTROL_HIT;
                ctrl_changes[ctrl_changed++] = e.key.keysym.scancode;
            }
        } else if(e.type == SDL_KEYUP) {
            ctrl_state[e.key.keysym.scancode] = PG_CONTROL_RELEASED;
            ctrl_changes[ctrl_changed++] = e.key.keysym.scancode;
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
        vec2_set(mouse_pos, mx, my);
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
            ctrl_state[c] = 0;
            break;
        }
    }
    vec2_set(mouse_motion, 0, 0);
    ctrl_changed = 0;
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


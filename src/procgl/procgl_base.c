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

int pg_init(int w, int h, int fullscreen, const char* window_title)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    printf("vsync default: %d\n", SDL_GL_GetSwapInterval());
    SDL_GL_SetSwapInterval(0);
    SDL_DisplayMode display;
    SDL_GetDesktopDisplayMode(0, &display);
    screen_w = fullscreen ? display.w : w;
    screen_h = fullscreen ? display.h : h;
    render_w = w;
    render_h = h;
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
static float framerate_update_interval = 0.2;
void pg_calc_framerate(float new_time)
{
    if(new_time >= framerate_last_update + framerate_update_interval) {
        framerate_last_update = new_time;
        framerate = 1.0f / (new_time - last_time);
    }
    last_time = new_time;
}

float pg_framerate(void)
{
    return framerate;
}

#include "procgl.h"

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
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
    glewExperimental = GL_TRUE;
    glewInit();
    GLenum err = glGetError();
    printf("Discarding routine GLEW error: %d\n", err);
}

void pg_deinit(void)
{
    SDL_GL_DeleteContext(pg_context);
    SDL_DestroyWindow(pg_window);
    SDL_Quit();
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

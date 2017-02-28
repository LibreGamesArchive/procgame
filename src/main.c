#include <stdio.h>
#include "procgl/procgl.h"
#include "game/game.h"

SDL_GLContext procgl_init(SDL_Window** window)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    *window = SDL_CreateWindow("Ludum Hadron Collider",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               800, 600, SDL_WINDOW_OPENGL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_ShowCursor(SDL_DISABLE);
    return SDL_GL_CreateContext(*window);
}

int main(int argc, char *argv[])
{
    SDL_Window* window;
    SDL_GLContext context = procgl_init(&window);
    glEnable(GL_CULL_FACE);
    glewExperimental = GL_TRUE;
    glewInit();
    GLenum err = glGetError();
    printf("Discarding routine GLEW error: %d\n", err);
    struct collider_state game;
    collider_init(&game);
    int user_exit = 0;
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    while (!user_exit)
    {
        /*  Get input   */
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) user_exit = 1;
        }
        /*  Set up the transformation matrix    */
        /*  Do The Thing!   */
        collider_update(&game);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        collider_draw(&game);
        SDL_GL_SwapWindow(window);
    }
    /*  Clean it all up */
    collider_deinit(&game);
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

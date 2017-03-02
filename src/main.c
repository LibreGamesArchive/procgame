#include <stdio.h>
#include "procgl/procgl.h"
#include "game/game.h"

int main(int argc, char *argv[])
{
    pg_init(800, 600, 0, "Ludum Hadron Collider");
    glEnable(GL_CULL_FACE);
    struct collider_state game;
    collider_init(&game);
    int user_exit = 0;
    while (!user_exit)
    {
        /*  Get input   */
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) user_exit = 1;
        }
        /*  Do The Thing!   */
        collider_update(&game);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        collider_draw(&game);
        pg_screen_swap();
    }
    /*  Clean it all up */
    collider_deinit(&game);
    pg_deinit();
    return 0;
}

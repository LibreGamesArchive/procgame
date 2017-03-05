#include <stdio.h>
#include <time.h>
#include "procgl/procgl.h"
#include "game/game.h"

int main(int argc, char *argv[])
{
    pg_init(1024, 576, 0, "Ludum Hadron Collider");
    srand(time(0));
    glEnable(GL_CULL_FACE);
    struct collider_state game;
    collider_init(&game);
    int user_exit = 0;
    while (game.state != LHC_EXIT)
    {
        /*  Get input   */
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

#include <stdio.h>
#include <time.h>
#include "procgl/procgl.h"
#include "game/game_fps.h"

int main(int argc, char *argv[])
{
    //static const char tree[] = "(union (union (CUBE 1 1 1) (CUBE 0.5 2 0.5)) (CYLINDER 0.25 2))";
    static const char tree[] = "(union (BOX 1 1 1) (CYLINDER 0.5 2))";
    struct pg_sdf_tree sdf;
    pg_sdf_tree_init(&sdf);
    pg_sdf_tree_parse(&sdf, tree, sizeof(tree));
    char printout[128] = {};
    pg_sdf_tree_print(&sdf, printout, 128);
    printf("%s\n", printout);
    /*  Read the options file   */
    FILE* config = fopen("./options.txt", "r");
    int w, h, fullscreen;
    float mouse_sens;
    fscanf(config, "x:%d\ny:%d\nfullscreen:%d\nmouse:%f",
           &w, &h, &fullscreen, &mouse_sens);
    /*  Init procgame   */
    pg_init(w, h, fullscreen, "procgame");
    glEnable(GL_CULL_FACE);
    srand(time(0));
    /*  Init example game   */
    struct pg_game_state game;
    fps_game_start(&game);
    /*  Main loop   */
    while(game.tick) {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float time = pg_time();
        pg_calc_framerate(time);
        pg_game_state_update(&game, time);
        pg_game_state_draw(&game);
        pg_screen_swap();
    }
    /*  Clean it all up */
    pg_game_state_deinit(&game);
    pg_deinit();
    return 0;
}

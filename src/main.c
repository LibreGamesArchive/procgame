#include <stdio.h>
#include <time.h>
#include "procgl/procgl.h"
#include "game/game_example.h"
#include "handle_crash.h"

int main(int argc, char *argv[])
{
    program_name = argv[0];
    set_signal_handler();
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
    example_game_start(&game);
    /*  Main loop   */
    while(game.running) {
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

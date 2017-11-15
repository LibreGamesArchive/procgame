#include <stdio.h>
#include <time.h>
#include "procgl/procgl.h"
#include "BORK_of_DOOM/bork.h"

int main(int argc, char *argv[])
{
    /*  Read the options file   */
    int w, h, fullscreen, opts_read;
    float mouse_sens;
    char* base_path = SDL_GetBasePath();
    char config_path[1024];
    snprintf(config_path, 1024, "%soptions.txt", base_path);
    FILE* config = fopen(config_path, "r");
    if(!config) {
        printf("Could not read options.txt, exiting.\n");
        return 0;
    }
    opts_read = fscanf(config, "x:%d\ny:%d\nfullscreen:%d\nmouse:%f",
                           &w, &h, &fullscreen, &mouse_sens);
    if(opts_read != 4) {
        printf("Bad options.txt, exiting.\n");
    }
    /*  Init procgame   */
    pg_init(w, h, fullscreen, "BORK of DOOM");
    glEnable(GL_CULL_FACE);
    srand(time(0));
    /*  Init BORK OF DOOM   */
    struct bork_game_core bork;
    struct pg_game_state game;
    bork_init(&bork, base_path);
    bork.fullscreen = fullscreen;
    bork.mouse_sensitivity = mouse_sens;
    bork_menu_start(&game, &bork);
    /*  Main loop   */
    while(game.running) {
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
    bork_deinit(&bork);
    pg_deinit();
    return 0;
}

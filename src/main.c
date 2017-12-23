#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "procgl/procgl.h"
#include "BORK_of_DOOM/bork.h"
#include "handle_crash.h"

int main(int argc, char *argv[])
{
    program_name = argv[0];
    set_signal_handler();
    /*  Read the options file   */
    char* base_path = SDL_GetBasePath();
    char config_path[1024];
    snprintf(config_path, 1024, "%soptions.txt", base_path);
    int w, h, fullscreen, invert_mouse, show_fps;
    float mouse_sens, joy_sens, gamma, music_vol, sfx_vol;
    FILE* config = fopen(config_path, "r");
    if(!config) {
        printf("Could not read options.txt, using defaults.\n");
        w = 800;
        h = 600;
        fullscreen = 0;
        gamma = 1.0f;
        mouse_sens = 0.001;
        joy_sens = 1.0f;
        invert_mouse = 0;
        music_vol = 1;
        sfx_vol = 1;
        show_fps = 0;
    } else {
        int opts_read = fscanf(config,
                            "x:%d\ny:%d\nfullscreen:%d\ngamma:%f\nmouse:%f\njoy:%f\n"
                            "invert_mouse:%d\nmusic_vol:%f\nsfx_vol:%f\nshow_fps:%d",
                            &w, &h, &fullscreen, &gamma, &mouse_sens, &joy_sens, &invert_mouse,
                            &music_vol, &sfx_vol, &show_fps);
        if(opts_read != 10) {
            printf("Bad options.txt, using defaults.\n");
            w = 800;
            h = 600;
            fullscreen = 0;
            gamma = 1.0f;
            mouse_sens = 0.001;
            joy_sens = 1.0f;
            invert_mouse = 0;
            music_vol = 1;
            sfx_vol = 1;
            show_fps = 0;
        }
    }
    /*  Init procgame   */
    pg_init(w, h, fullscreen, "The Communist Dogifesto");
    glEnable(GL_CULL_FACE);
    srand(time(0));
    /*  Init BORK OF DOOM   */
    struct bork_game_core bork;
    struct pg_game_state game;
    bork_init(&bork, base_path);
    bork_set_gamma(&bork, gamma);
    bork_set_music_volume(&bork, music_vol);
    bork_set_sfx_volume(&bork, sfx_vol);
    bork.invert_y = invert_mouse;
    bork.show_fps = show_fps;
    bork.fullscreen = fullscreen;
    bork.mouse_sensitivity = mouse_sens;
    bork.joy_sensitivity = joy_sens;
    if(argc >= 2 && strcmp(argv[1], "edit") == 0) {
        bork_editor_start(&game, &bork);
    } else {
        bork_menu_start(&game, &bork);
    }
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
    pg_deinit();
    bork_deinit(&bork);
    return 0;
}

#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "linmath.h"
#include "procgl/vertex.h"
#include "procgl/shader.h"
#include "procgl/texture.h"
#include "procgl/model.h"
#include "procgl/shape.h"

SDL_GLContext procgl_init(SDL_Window** window)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    *window = SDL_CreateWindow("OpenGL",
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               800, 600, SDL_WINDOW_OPENGL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
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
    /*  Load and initialize the shaders */
    struct pg_shader shader_2d;
    pg_shader_2d(&shader_2d);
    struct pg_shader shader_3d;
    pg_shader_3d(&shader_3d);
    pg_shader_3d_set_fog(&shader_3d,
                            (vec2){ 1, 20 }, (vec3){ 0.67, 0.58, 0.42 });
    pg_shader_3d_set_light(&shader_3d,
                            (vec3){ 0, -1, 0.2 },
                            (vec3){ 0.67 * 3, 0.58 * 3, 0.42 * 3 },
                            (vec3){ 0.2, 0.2, 0.2 });
    /*  Make a viewer object to view the things */
    struct pg_viewer test_view;
    pg_viewer_init(&test_view, (vec3){ 0, 0, 3 }, (vec2){ 0, 0 },
                   (vec2){ 800, 600 }, (vec2){ 0.1, 100 });
    /*  A test shape and a test model   */
    struct pg_shape test_shape;
    pg_shape_rect(&test_shape);
    pg_shape_buffer(&test_shape, &shader_2d);
    struct pg_model test_model;
    pg_model_cube(&test_model);
    pg_model_precalc_verts(&test_model);
    pg_model_buffer(&test_model, &shader_3d);
    int user_exit = 0;
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    while (!user_exit)
    {
        /*  Get input   */
        int mouse_x, mouse_y;
        SDL_Event windowEvent;
        while(SDL_PollEvent(&windowEvent))
        {
            if (windowEvent.type == SDL_QUIT) user_exit = 1;
        }
        SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
        /*  Handle input; get the current view angle, add mouse motion to it  */
        pg_viewer_set(&test_view, test_view.pos,
            (vec2){ test_view.dir[0] + mouse_x * 0.001,
                    test_view.dir[1] + mouse_y * 0.001 });
        /*  Set up the transformation matrix    */
        mat4 shape_transform;
        mat4_identity(shape_transform);
        mat4 model_transform;
        mat4_identity(model_transform);
        /*  A bit down, to the right, and ahead of the camera   */
        mat4_translate(model_transform, 2, 6, 0);
        /*  Do The Thing!   */
        glEnable(GL_DEPTH_TEST);
        glDepthMask(1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        pg_shader_begin(&shader_3d, &test_view);
        pg_model_begin(&test_model);
        pg_model_draw(&test_model, &shader_3d, model_transform);
        pg_shader_begin(&shader_2d, NULL);
        pg_shape_begin(&test_shape);
        pg_shape_draw(&test_shape, &shader_2d, shape_transform);
        SDL_GL_SwapWindow(window);
    }
    /*  Clean it all up */
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

#include <stdio.h>
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "lodepng.h"
#include "linmath.h"
#include "procgl/noise1234.h"

#include "procgl/renderer.h"
#include "procgl/texture.h"
#include "procgl/model.h"
#include "procgl/wave.h"
#include "procgl/terrain.h"

GLuint load_texture(const char* filename)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    unsigned w, h;
    unsigned char* pixels;
    lodepng_decode32_file(&pixels, &w, &h, filename);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    free(pixels);
    return tex;
}

static void color_texture(struct procgl_texture* tex)
{
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            float h = tex->normals[x + y * tex->w].h;
            tex->pixels[x + y * tex->w] = (struct procgl_texture_pixel) {
                h * 0.18 +  255 * 0.66, h * 0.18 + 255 * 0.20,
                h * 0.27 + 255 * 0.10, 1 };
        }
    }
}

static void color_texture_rock(struct procgl_texture* tex)
{
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            float h = tex->normals[x + y * tex->w].h;
            tex->pixels[x + y * tex->w] = (struct procgl_texture_pixel) {
                h * 0.40 +  255 * 0.20, h * 0.18 + 255 * 0.20,
                h * 0.18 + 255 * 0.20, 1 };
        }
    }
}

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_Window* window = SDL_CreateWindow("OpenGL",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          800, 600, SDL_WINDOW_OPENGL);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_GLContext context = SDL_GL_CreateContext(window);
    GLenum err = glGetError();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glewExperimental = GL_TRUE;
    glewInit();
    err = glGetError();
    printf("Discarding routine GLEW error: %d\n", err);
    /*  Building the renderer; handles viewport, lighting, fog  */
    struct procgl_renderer test_rend;
    procgl_renderer_init(&test_rend, "src/procgl/terrain_vert.glsl", "src/procgl/terrain_frag.glsl");
    procgl_renderer_set_view(&test_rend, (vec3){ 0, 0, 3 }, (vec2){ 0, 0 });
    procgl_renderer_set_sun(&test_rend, (vec3){ 0, -1, 0.2 },
                            (vec3){ 0.67 * 3, 0.58 * 3, 0.42 * 3 },
                            (vec3){ 0.2, 0.2, 0.2 });
    procgl_renderer_set_fog(&test_rend, (vec2){ 1, 20 },
                            (vec3){ 0.67, 0.58, 0.42 });
    /*  procgl_wave demonstration; test_comp is a composite wave of two
        sine waves, one at double frequency and half scale. It is later used
        as the source for terrain demonstration */
    struct procgl_wave sine_1;
    struct procgl_wave sine_2;
    procgl_wave_init_sine(&sine_1);
    procgl_wave_init_sine(&sine_2);
    sine_1.frequency[0] = 10;
    sine_1.frequency[1] = 10;
    sine_2.scale = 0.5;
    sine_2.frequency[0] = 20;
    sine_2.frequency[1] = 20;
    struct procgl_wave test_comp;
    procgl_wave_composite(&test_comp, &sine_1, &sine_2, 0.5, procgl_wave_mix_add);
    test_comp.scale = 5;
    /*  procgl_terrain demonstration; it can use a procgl_wave to generate
        a set of 3d vertices; it samples the range (-1, 1) so phasing must be
        included in the provided wave   */
    struct procgl_terrain test_terr;
    procgl_terrain_init(&test_terr, 64, 64, 2);
    procgl_terrain_from_wave(&test_terr, &test_comp, (vec3){ 1, 1, 1 });
    procgl_model_precalc_verts(&test_terr.model);
    procgl_terrain_buffer(&test_terr);
    /*  procgl_texture demonstration; these are placed in the renderer's
        texture slots 0-3 to be used when rendering the terrain */
    struct procgl_texture tex_regolith;
    procgl_texture_init(&tex_regolith, 64, 64);
    procgl_texture_perlin(&tex_regolith, 0.5, 0.5, 2.1, 10.28);
    procgl_texture_generate_normals(&tex_regolith);
    color_texture(&tex_regolith);
    procgl_texture_buffer(&tex_regolith);
    struct procgl_texture tex_regolith_d;
    procgl_texture_init(&tex_regolith_d, 64, 64);
    procgl_texture_perlin(&tex_regolith_d, 0.5, 0.5, 3.2, 5.2);
    procgl_texture_generate_normals(&tex_regolith_d);
    color_texture(&tex_regolith_d);
    procgl_texture_buffer(&tex_regolith_d);
    struct procgl_texture tex_rock;
    procgl_texture_init(&tex_rock, 64, 64);
    procgl_texture_perlin(&tex_rock, 30.5, 30.5, 32.5, 32.5);
    procgl_texture_generate_normals(&tex_rock);
    color_texture_rock(&tex_rock);
    procgl_texture_buffer(&tex_rock);
    struct procgl_texture tex_rock_d;
    procgl_texture_init(&tex_rock_d, 64, 64);
    procgl_texture_perlin(&tex_rock_d, 30.5, 30.5, 310.5, 310.5);
    procgl_texture_generate_normals(&tex_rock_d);
    color_texture_rock(&tex_rock_d);
    procgl_texture_buffer(&tex_rock_d);
    /*  Using the special terrain function to put the texture in the texture
        slot, and also set the terrain texture info (logical texture height,
        texture scale, and the detail texture weight)   */
    procgl_texture_use_terrain(&tex_regolith, &test_rend, 0, 0, 0.25, 0.5);
    procgl_texture_use_terrain(&tex_rock_d, &test_rend, 1, 0, 0.5, 0.25);
    procgl_texture_use_terrain(&tex_rock, &test_rend, 2, 0, 0.2, 0.5);
    procgl_texture_use_terrain(&tex_rock_d, &test_rend, 3, 0, 0.4, 0.5);
    /*  procgl_model demonstration; WIP for now */
    struct procgl_model test_model;
    procgl_model_init(&test_model);
    procgl_model_cube(&test_model);
    procgl_model_split_tris(&test_model);
    procgl_model_generate_texture(&test_model, NULL, 0, 0);
    procgl_model_precalc_verts(&test_model);
    procgl_model_buffer(&test_model);
    /*  Checking if there were any errors in all the procgl code    */
    err = glGetError();
    printf("procgl error:%d\n", err);
    float angle = 0;
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
        vec2 view_angle;
        procgl_renderer_get_view(&test_rend, NULL, view_angle);
        procgl_renderer_set_view(&test_rend, NULL,
            (vec2){ view_angle[0] + mouse_x * 0.001,
                    view_angle[1] + mouse_y * 0.001 });
        /*  Set up the transformation matrix    */
        angle += 0.01;
        mat4 model_transform;
        mat4_translate(model_transform, 0, -3, 2);
        mat4_rotate(model_transform, model_transform, 1, 1, 0, angle);
        mat4 terrain_transform;
        mat4_translate(terrain_transform, 0, 0, 2);
        /*  Do The Thing    */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        procgl_renderer_begin_terrain(&test_rend);
        procgl_terrain_begin(&test_terr, &test_rend);
        procgl_terrain_draw(&test_rend, &test_terr, terrain_transform);
        procgl_renderer_begin_model(&test_rend);
        procgl_model_begin(&test_model, &test_rend);
        procgl_model_draw(&test_rend, &test_model, model_transform);
        SDL_GL_SwapWindow(window);
    }
    /*  Clean it all up */
    procgl_renderer_deinit(&test_rend);
    procgl_model_deinit(&test_model);
    procgl_terrain_deinit(&test_terr);
    procgl_texture_deinit(&tex_rock);
    procgl_texture_deinit(&tex_rock_d);
    procgl_texture_deinit(&tex_regolith);
    procgl_texture_deinit(&tex_regolith_d);
    SDL_DestroyWindow(window);
    SDL_GL_DeleteContext(context);
    SDL_Quit();
    return 0;
}

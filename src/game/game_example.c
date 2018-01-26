#include <stdlib.h>
#include <limits.h>
#include "procgl/procgl.h"

struct example_game_assets {
    struct pg_texture font;
    struct pg_model quad;
};

struct example_game_data {
    struct example_game_assets assets;
    struct pg_renderer rend;
    /*  Swap-buffers
     *  draw to 0, read from 1; then draw to 1, read from 0; repeat */
    struct pg_texture pptex[2];
    struct pg_renderbuffer ppbuf[2];
    struct pg_rendertarget target;
    /*  Draw passes */
    struct pg_renderpass test_pass;
    struct pg_renderpass post_sine;
    struct pg_renderpass post_blur;
    struct pg_renderpass text_pass;
    vec2 player_pos, player_vel;
};

void example_game_update(struct pg_game_state*, float);
void example_game_tick(struct pg_game_state*);
void example_game_draw(struct pg_game_state*);
void example_game_deinit(void*);

static int drawformat_2d(const struct pg_uniform* unis, const struct pg_shader* shader,
                         const mat4* mats, const GLint* idx)
{
    glUniformMatrix4fv(idx[0], 1, GL_FALSE, *unis[0].m);
    glUniform4f(idx[1], unis[1].f[0][0], unis[1].f[0][1],
                        unis[1].f[0][2], unis[1].f[0][3]);
    glUniform2f(idx[2], unis[2].f[0][0], unis[2].f[0][1]);
    return 3;
}

static int drawformat_sine(const struct pg_uniform* unis, const struct pg_shader* shader,
                         const mat4* mats, const GLint* idx)
{
    glUniform1f(idx[0], unis[0].f[0][0]);
    glUniform1f(idx[1], unis[0].f[0][1]);
    glUniform1f(idx[2], unis[0].f[0][2]);
    glUniform1i(idx[3], (int)unis[0].f[0][3]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return 1;
}

static int drawformat_blur(const struct pg_uniform* unis, const struct pg_shader* shader,
                         const mat4* mats, const GLint* idx)
{
    glUniform2f(idx[0], unis[0].f[0][0], unis[0].f[0][1]);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    return 1;
}

static int drawformat_text(const struct pg_uniform* unis, const struct pg_shader* shader,
                         const mat4* mats, const GLint* idx)
{
    glUniformMatrix4fv(idx[0], 1, GL_FALSE, *mats[PG_VIEW_MATRIX]);
    glUniform1uiv(idx[1], 16, (unsigned int*)unis[0].str);
    glUniform4f(idx[2], unis[1].f[0][0], unis[1].f[0][1],
                        unis[1].f[0][2], unis[1].f[0][3]);
    glUniform4f(idx[3], unis[1].f[1][0], unis[1].f[1][1],
                        unis[1].f[1][2], unis[1].f[1][3]);
    glUniform2f(idx[4], unis[1].f[2][0], unis[1].f[2][1]);
    int len = MAX(64, (int)unis[1].f[2][2]);
    glDrawArrays(GL_TRIANGLES, 0, len * 6);
    return 2;
}

void example_game_start(struct pg_game_state* state)
{
    pg_game_state_init(state, pg_time(), 30, 3);
    struct example_game_data* d = malloc(sizeof(*d));
    /*  Basic centered quad */
    pg_model_init(&d->assets.quad);
    pg_model_quad_2d(&d->assets.quad, (vec2){ 1, 1 });
    mat4 transform;
    mat4_identity(transform);
    mat4_scale_aniso(transform, transform, 1, 1, 0);
    pg_model_transform(&d->assets.quad, transform);
    pg_model_buffer(&d->assets.quad);
    /*  The back-buffer */
    int w, h;
    pg_screen_size(&w, &h);
    pg_texture_init(&d->pptex[0], w, h, PG_UBVEC4);
    pg_texture_init(&d->pptex[1], w, h, PG_UBVEC4);
    pg_renderbuffer_init(&d->ppbuf[0]);
    pg_renderbuffer_init(&d->ppbuf[1]);
    pg_renderbuffer_attach(&d->ppbuf[0], &d->pptex[0], 0, GL_COLOR_ATTACHMENT0);
    pg_renderbuffer_attach(&d->ppbuf[1], &d->pptex[1], 0, GL_COLOR_ATTACHMENT0);
    pg_rendertarget_init(&d->target, &d->ppbuf[0], &d->ppbuf[1]);
    /*  Load a font texture */
    pg_texture_init_from_file(&d->assets.font, "res/font_8x8.png");
    pg_texture_set_atlas(&d->assets.font, 8, 8);
    /*  Initialize the renderer and two render passes   */
    pg_renderer_init(&d->rend);
    pg_renderpass_init(&d->rend, &d->test_pass, "2d", GL_COLOR_BUFFER_BIT);
    pg_renderpass_init(&d->rend, &d->text_pass, "text", 0);
    pg_renderpass_init(&d->rend, &d->post_sine, "post_sine", GL_COLOR_BUFFER_BIT);
    pg_renderpass_init(&d->rend, &d->post_blur, "post_blur", GL_COLOR_BUFFER_BIT);
    pg_renderpass_target(&d->test_pass, &d->target, 0);
    pg_renderpass_target(&d->text_pass, &d->target, 0);
    pg_renderpass_target(&d->post_sine, &d->target, 1);
    pg_renderpass_target(&d->post_blur, &d->target, 1);
    /*  Basic 2d pass texture and drawformat    */
    pg_renderpass_model(&d->test_pass, &d->assets.quad);
    pg_renderpass_texture(&d->test_pass, 1, &d->assets.font);
    pg_renderpass_drawformat(&d->test_pass, drawformat_2d, 3,
                              "pg_matrix_model", "pg_tex_rect_0",
                              "sprite_size");
    /*  Text pass texture and drawformat    */
    pg_renderpass_texture(&d->text_pass, 1, &d->assets.font);
    pg_renderpass_uniform(&d->text_pass, "font_pitch", PG_UINT,
                          &PG_UNIFORM_UINT({8}));
    pg_renderpass_uniform(&d->text_pass, "glyph_size", PG_VEC4,
                          &PG_UNIFORM_FLOAT({ 8.0f / 64.0f, 8.0f / 96.0f, 1, 1 }));
    pg_renderpass_drawformat(&d->text_pass, drawformat_text, 5,
            "pg_matrix_mvp", "chars", "style", "color", "dir");
    /*  Post-effect setup and drawformat    */
    pg_renderpass_drawformat(&d->post_sine, drawformat_sine, 4,
            "amplitude", "frequency", "phase", "axis");
    /*  Blur effect setup and drawformat    */
    pg_renderpass_uniform(&d->post_blur, "resolution", PG_VEC2,
                &PG_UNIFORM_FLOAT({ w, h }));
    pg_renderpass_drawformat(&d->post_blur, drawformat_blur, 1, "blur_dir");
    /*  Set up a couple test draw calls */
    struct pg_uniform test_draw[3] = {};
    mat4_identity(test_draw[0].m);
    test_draw[1] = PG_UNIFORM_FLOAT({ 0, 0, 1.0f, 1.0f });
    test_draw[2] = PG_UNIFORM_FLOAT({ 0.5f, 1.0f });
    pg_renderpass_add_draw(&d->test_pass, 3, test_draw);
    mat4_translate(test_draw[0].m, 0.5, 0, 0);
    test_draw[1] = PG_UNIFORM_FLOAT({ 0, 0, 0.5f, 0.5f });
    test_draw[2] = PG_UNIFORM_FLOAT({ 0.5f, 1.0f });
    pg_renderpass_add_draw(&d->test_pass, 3, test_draw);
    /*  Set up a text call  */
    test_draw[0] = PG_UNIFORM_STRING("HELLO, WORLD!");
    test_draw[1] = PG_UNIFORM_FLOAT(
            { -1, -0.5, 0.1, 1.2 }, { 1, 0, 0, 1 }, { 1, 1, (float)strlen("HELLO, WORLD!") });
    pg_renderpass_add_draw(&d->text_pass, 2, test_draw);
    /*  Set up a post-effect call   */
    test_draw[0] = PG_UNIFORM_FLOAT({ 0.005f, 50.0f, 0.0f, 1.0f });
    test_draw[1] = PG_UNIFORM_FLOAT({ 0.002f, 400.0f, 0.0f, 1.0f });
    test_draw[2] = PG_UNIFORM_FLOAT({ 0.05f, 1.0f, 1.0f, 0.0f });
    pg_renderpass_add_draw(&d->post_sine, 1, test_draw);
    /*  Set up blur effect calls    */
    test_draw[0] = PG_UNIFORM_FLOAT({ 1.0f, 0.0f });
    test_draw[1] = PG_UNIFORM_FLOAT({ 0.0f, 1.0f });
    pg_renderpass_add_draw(&d->post_blur, 2, test_draw);
    /*  Add the rendergroup to the first pass   */
    /*  Add the two passes to the renderer  */
    ARR_PUSH(d->rend.passes, &d->test_pass);
    ARR_PUSH(d->rend.passes, &d->text_pass);
    ARR_PUSH(d->rend.passes, &d->post_sine);
    ARR_PUSH(d->rend.passes, &d->post_blur);
    mat4_ortho(d->test_pass.mats[PG_VIEW_MATRIX], -1, 1, 1, -1, -1, 1);
    mat4_ortho(d->text_pass.mats[PG_VIEW_MATRIX], -1, 1, 1, -1, -1, 1);
    pg_renderpass_uniform(&d->test_pass, "ambient_color", PG_VEC3,
                          &PG_UNIFORM_FLOAT({ 1, 1, 1 }));
    pg_renderpass_uniform(&d->test_pass, "color_mod", PG_VEC4,
                          &PG_UNIFORM_FLOAT({ 1, 1, 1, 1 }));
    pg_renderpass_uniform(&d->test_pass, "tex_weight", PG_FLOAT,
                          &PG_UNIFORM_FLOAT({ 1 }));
    /*  Fill in state structure */
    state->data = d;
    state->tick = example_game_tick;
    state->draw = example_game_draw;
    state->deinit = example_game_deinit;
}

void example_game_tick(struct pg_game_state* state)
{
    struct example_game_data* d = state->data;
    pg_poll_input();
    if(pg_user_exit()) state->running = 0;
    if(pg_check_input(SDL_SCANCODE_LEFT, PG_CONTROL_HELD)) {
        d->player_vel[0] -= 0.01;
    }
    if(pg_check_input(SDL_SCANCODE_RIGHT, PG_CONTROL_HELD)) {
        d->player_vel[0] += 0.01;
    }
    if(pg_check_input(SDL_SCANCODE_UP, PG_CONTROL_HELD)) {
        d->player_vel[1] -= 0.01;
    }
    if(pg_check_input(SDL_SCANCODE_DOWN, PG_CONTROL_HELD)) {
        d->player_vel[1] += 0.01;
    }
    vec2_add(d->player_pos, d->player_pos, d->player_vel);
    vec2_scale(d->player_vel, d->player_vel, 0.8);
    pg_flush_input();
}

void example_game_draw(struct pg_game_state* state)
{
    struct example_game_data* d = state->data;
    float t = state->tick_over;
    pg_screen_dst();
    vec2 vel_lerp = { d->player_vel[0] * t, d->player_vel[1] * t };
    vec2 pos = { d->player_pos[0] + vel_lerp[0],
                 d->player_pos[1] + vel_lerp[1] };
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    pg_renderer_draw_frame(&d->rend);
}

void example_game_deinit(void* data)
{
    struct example_game_data* d = data;
    pg_texture_deinit(&d->assets.font);
    pg_model_deinit(&d->assets.quad);
    free(d);
}

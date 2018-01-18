#include "../procgl.h"
#include "procgl/shaders/shaders.h"

#ifdef PROCGL_STATIC_SHADERS
#include "procgl/shaders/screen.glsl.h"
#include "procgl/shaders/post_gamma.glsl.h"
#endif

struct gamma_data {
    GLuint uni_gamma;
    float gamma;
};

static void gamma_pre(struct pg_postproc* pp)
{
    struct gamma_data* d = pp->data;
    glUniform1f(d->uni_gamma, d->gamma);
}

void pg_postproc_gamma(struct pg_postproc* pp)
{
#ifdef PROCGL_STATIC_SHADERS
    pg_postproc_load_static(pp, screen_vert_glsl, screen_vert_glsl_len,
                            post_gamma_frag_glsl, post_gamma_frag_glsl_len,
                            "color", "resolution");
#else
    pg_postproc_load(pp, SHADER_BASE_DIR "screen_vert.glsl",
                         SHADER_BASE_DIR "post_gamma_frag.glsl",
                         "color", "resolution");
#endif
    struct gamma_data* d = malloc(sizeof(struct gamma_data));
    d->gamma = 2.2;
    d->uni_gamma = glGetUniformLocation(pp->prog, "gamma");
    pp->data = d;
    pp->pre = gamma_pre;
    pp->deinit = free;
}

void pg_postproc_gamma_set(struct pg_postproc* pp, float gamma)
{
    struct gamma_data* d = pp->data;
    d->gamma = gamma;
}


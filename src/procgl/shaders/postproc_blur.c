#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include "procgl/ext/linmath.h"
#include "procgl/postproc.h"
#include "procgl/texture.h"
#include "procgl/shader.h"
#include "procgl/shaders/shaders.h"

struct blur_data {
    int state_dir;
    vec2 state_scale;
    GLuint uni_dir;
    GLuint uni_scale;
};

static void blur_pre(struct pg_postproc* pp)
{
    struct blur_data* d = pp->data;
    glUniform1i(d->uni_dir, d->state_dir);
    glUniform2f(d->uni_scale, d->state_scale[0], d->state_scale[1]);
}

void pg_postproc_blur(struct pg_postproc* pp, enum pg_postproc_blur_level level)
{
    switch(level) {
    case PG_BLUR3:
        pg_postproc_load(pp, "src/procgl/shaders/screen_vert.glsl",
                             "src/procgl/shaders/post_blur3_frag.glsl",
                             "color", "resolution");
        break;
    case PG_BLUR5:
        pg_postproc_load(pp, "src/procgl/shaders/screen_vert.glsl",
                             "src/procgl/shaders/post_blur5_frag.glsl",
                             "color", "resolution");
        break;
    case PG_BLUR7:
        pg_postproc_load(pp, "src/procgl/shaders/screen_vert.glsl",
                             "src/procgl/shaders/post_blur7_frag.glsl",
                             "color", "resolution");
        break;
    }
    struct blur_data* d = malloc(sizeof(struct blur_data));
    d->state_dir = 0;
    vec2_set(d->state_scale, 1, 1);
    d->uni_dir = glGetUniformLocation(pp->prog, "blur_dir");
    d->uni_scale = glGetUniformLocation(pp->prog, "blur_scale");
    pp->data = d;
    pp->pre = blur_pre;
    pp->deinit = free;
}

void pg_postproc_blur_dir(struct pg_postproc* pp, int dir)
{
    struct blur_data* d = pp->data;
    d->state_dir = dir;
}

void pg_postproc_blur_scale(struct pg_postproc* pp, vec2 scale)
{
    struct blur_data* d = pp->data;
    vec2_dup(d->state_scale, scale);
}

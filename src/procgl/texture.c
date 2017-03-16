#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>

#include "ext/noise1234.h"
#include "ext/lodepng.h"
#include "ext/linmath.h"
#include "wave.h"
#include "heightmap.h"
#include "texture.h"




void pg_texture_init_from_file(struct pg_texture* tex,
                               const char* diffuse_file, const char* light_file)
{
    unsigned w0, h0, w1, h1;
    lodepng_decode32_file(&tex->diffuse, &w0, &h0, diffuse_file);
    tex->w = w0;
    tex->h = h0;
    tex->diffuse_slot = 0;
    tex->light_slot = 0;
    glGenTextures(1, &tex->diffuse_gl);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->diffuse_gl);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, tex->diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(light_file) {
        lodepng_decode32_file(&tex->light, &w1, &h1, light_file);
        if(w0 != w1 || h0 != h1) {
            printf("Warning: colormap and normalmap size mismatch:\n"
                   "    colormap: %s\n    normalmap: %s\n",
                   diffuse_file, light_file);
        }
        glGenTextures(1, &tex->light_gl);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->light_gl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, tex->light);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        tex->light = NULL;
        tex->light_slot = -1;
    }
}

void pg_texture_init(struct pg_texture* tex, int w, int h)
{
    tex->diffuse = calloc(w * h, sizeof(*tex->diffuse));
    tex->light = calloc(w * h, sizeof(*tex->light));
    tex->w = w;
    tex->h = h;
    tex->diffuse_slot = 0;
    tex->light_slot = 0;
    glGenTextures(1, &tex->diffuse_gl);
    glGenTextures(1, &tex->light_gl);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->diffuse_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->light_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void pg_texture_deinit(struct pg_texture* tex)
{
    glDeleteTextures(1, &tex->diffuse_gl);
    free(tex->diffuse);
    if(tex->light) {
        glDeleteTextures(1, &tex->light_gl);
        free(tex->light);
    }
}

void pg_texture_bind(struct pg_texture* tex, int diffuse_slot, int light_slot)
{
    tex->diffuse_slot = diffuse_slot;
    tex->light_slot = light_slot;
    if(diffuse_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + diffuse_slot);
        glBindTexture(GL_TEXTURE_2D, tex->diffuse_gl);
    }
    if(tex->light && light_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + light_slot);
        glBindTexture(GL_TEXTURE_2D, tex->light_gl);
    }
}

void pg_texture_buffer(struct pg_texture* tex)
{
    if(tex->diffuse_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + tex->diffuse_slot);
        glBindTexture(GL_TEXTURE_2D, tex->diffuse_gl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, tex->diffuse);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    if(tex->light && tex->light_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + tex->light_slot);
        glBindTexture(GL_TEXTURE_2D, tex->light_gl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, tex->light);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void pg_texture_generate_normals(struct pg_texture* tex,
                                 struct pg_heightmap* hmap, float intensity)
{
    if(!tex->light) return;
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            float u, d, r, l;
            u = pg_heightmap_get_height(hmap, x, y - 1) * intensity;
            d = pg_heightmap_get_height(hmap, x, y + 1) * intensity;
            l = pg_heightmap_get_height(hmap, x - 1, y) * intensity;
            r = pg_heightmap_get_height(hmap, x + 1, y) * intensity;
            vec3 normal = { r - l, d - u, 2.0f };
            vec3_normalize(normal, normal);
            pg_texel_set(tex->light[x + y * tex->w],
                (normal[0] + 1) / 2 * 255,
                (normal[1] + 1) / 2 * 255,
                (normal[2] + 1) / 2 * 128 + 127,
                tex->light[x + y * tex->w][3]);
        }
    }
}

void pg_texture_set_atlas(struct pg_texture* tex, int frame_w, int frame_h)
{
    tex->frame_w = frame_w;
    tex->frame_h = frame_h;
}

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>

#include "ext/noise1234.h"
#include "ext/lodepng.h"
#include "ext/linmath.h"
#include "texture.h"




void pg_texture_init_from_file(struct pg_texture* tex,
                               const char* color_file, const char* normal_file,
                               int color_slot, int normal_slot)
{
    unsigned w0, h0, w1, h1;
    lodepng_decode32_file(&tex->pixels, &w0, &h0, color_file);
    tex->w = w0;
    tex->h = h0;
    tex->color_slot = color_slot;
    glGenTextures(1, &tex->pixels_gl);
    glActiveTexture(GL_TEXTURE0 + color_slot);
    glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, tex->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(normal_file) {
        tex->normal_slot = normal_slot;
        lodepng_decode32_file(&tex->normals, &w1, &h1, normal_file);
        if(w0 != w1 || h0 != h1) {
            printf("Warning: colormap and normalmap size mismatch:\n"
                   "    colormap: %s\n    normalmap: %s\n",
                   color_file, normal_file);
        }
        glGenTextures(1, &tex->normals_gl);
        glActiveTexture(GL_TEXTURE0 + normal_slot);
        glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, tex->normals);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        tex->normals = NULL;
        tex->normal_slot = -1;
    }
}

void pg_texture_init(struct pg_texture* tex, int w, int h,
                     int color_slot, int normal_slot)
{
    tex->pixels = calloc(w * h, sizeof(*tex->pixels));
    tex->normals = calloc(w * h, sizeof(*tex->normals));
    tex->w = w;
    tex->h = h;
    tex->color_slot = color_slot;
    tex->normal_slot = normal_slot;
    glGenTextures(1, &tex->pixels_gl);
    glGenTextures(1, &tex->normals_gl);
    glActiveTexture(GL_TEXTURE0 + color_slot);
    glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0 + normal_slot);
    glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void pg_texture_deinit(struct pg_texture* tex)
{
    glDeleteTextures(1, &tex->pixels_gl);
    free(tex->pixels);
    if(tex->normals) {
        glDeleteTextures(1, &tex->normals_gl);
        free(tex->normals);
    }
}

void pg_texture_bind(struct pg_texture* tex, int color_slot, int normal_slot)
{
    if(color_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + color_slot);
        glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    }
    if(tex->normals && normal_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + normal_slot);
        glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
    }
}

void pg_texture_buffer(struct pg_texture* tex)
{
    if(tex->color_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + tex->color_slot);
        glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, tex->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    if(tex->normals && tex->normal_slot >= 0) {
        glActiveTexture(GL_TEXTURE0 + tex->normal_slot);
        glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, tex->normals);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

void pg_texture_perlin(struct pg_texture* tex,
                           float x1, float y1, float x2, float y2)
{
    if(!tex->normals) return;
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            float x_ = (float)x / tex->w;
            float y_ = (float)y / tex->h;
            float dx = x2 - x1;
            float dy = y2 - y1;
            float nx = x1 + cos(x_ * 2 * M_PI) * dx / (2 * M_PI);
            float ny = y1 + cos(y_ * 2 * M_PI) * dy / (2 * M_PI);
            float nz = x1 + sin(x_ * 2 * M_PI) * dx / (2 * M_PI);
            float nw = y1 + sin(y_ * 2 * M_PI) * dy / (2 * M_PI);
            float noise = noise4(nx, ny, nz, nw) * 0.5 + 0.5;
            tex->normals[x + y * tex->w].h = (unsigned char)(noise * 256);
        }
    }
}

void pg_texture_shitty(struct pg_texture* tex)
{
    if(!tex->normals) return;
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            vec2 xy = {x, y};
            vec2 wh = {(float)tex->w / 2, (float)tex->h / 2};
            vec2 diff;
            vec2_sub(diff, wh, xy);
            int dist = abs((int)vec2_len(diff)) * 20;
            if(dist > 255) dist = 255;
            tex->normals[x + y * tex->w].h = 255 - dist;
        }
    }
}

static unsigned pg_texture_height(struct pg_texture* tex, int x, int y)
{
    if(x < 0) x = tex->w - (-x) % tex->w;
    else if(x >= tex->w) x = x % tex->w;
    if(y < 0) y = tex->h - (-y) % tex->h;
    else if(y >= tex->h) y = y % tex->h;
    return tex->normals[x + y * tex->w].h;
}

void pg_texture_generate_normals(struct pg_texture* tex, float intensity)
{
    if(!tex->normals) return;
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            float u, d, r, l;
            u = (float)pg_texture_height(tex, x, y - 1) / 256 * intensity;
            d = (float)pg_texture_height(tex, x, y + 1) / 256 * intensity;
            l = (float)pg_texture_height(tex, x - 1, y) / 256 * intensity;
            r = (float)pg_texture_height(tex, x + 1, y) / 256 * intensity;
            vec3 normal = { r - l, d - u, 2.0f };
            vec3_normalize(normal, normal);
            tex->normals[x + y * tex->w] = (struct pg_texture_normal) {
                (normal[0] + 1) / 2 * 256,
                (normal[1] + 1) / 2 * 256,
                (normal[2] + 1) / 2 * 128 + 127,
                tex->normals[x + y * tex->w].h };
        }
    }
}

void pg_texture_set_atlas(struct pg_texture* tex, int frame_w, int frame_h)
{
    tex->frame_w = frame_w;
    tex->frame_h = frame_h;
}

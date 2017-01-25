#include "renderer.h"
#include "texture.h"

#include "noise1234.h"

void procgl_texture_init(struct procgl_texture* tex, int w, int h)
{
    tex->pixels = calloc(w * h, sizeof(*tex->pixels));
    tex->normals = calloc(w * h, sizeof(*tex->normals));
    tex->w = w;
    tex->h = h;
    glGenTextures(1, &tex->pixels_gl);
    glGenTextures(1, &tex->normals_gl);
    glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void procgl_texture_deinit(struct procgl_texture* tex)
{
    glDeleteTextures(1, &tex->pixels_gl);
    glDeleteTextures(1, &tex->normals_gl);
    free(tex->pixels);
    free(tex->normals);
}
void procgl_texture_buffer(struct procgl_texture* tex)
{
    glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, tex->pixels);
    glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->w, tex->h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, tex->normals);
}

void procgl_texture_use(struct procgl_texture* tex,
                        struct procgl_renderer* rend, int slot)
{
    glActiveTexture(GL_TEXTURE0 + (slot * 3));
    glBindTexture(GL_TEXTURE_2D, tex->pixels_gl);
    glActiveTexture(GL_TEXTURE1 + (slot * 3));
    glBindTexture(GL_TEXTURE_2D, tex->normals_gl);
    rend->tex_slots[0][slot] = slot + 3;
    rend->tex_slots[1][slot] = slot + 3 + 1;
    rend->tex_slots[2][slot] = slot + 3 + 2;

}

void procgl_texture_use_terrain(struct procgl_texture* tex,
                                struct procgl_renderer* rend, int slot,
                                float height_mod, float scale,
                                float detail_weight)
{
    procgl_texture_use(tex, rend, slot);
    rend->shader_terrain.data.height_mod[slot] = height_mod;
    rend->shader_terrain.data.scale[slot] = scale;
    if(slot & 1) {
        rend->shader_terrain.data.detail_weight[slot >> 1] = detail_weight;
    }
}

void procgl_texture_perlin(struct procgl_texture* tex,
                           float x1, float y1, float x2, float y2)
{
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

void procgl_texture_shitty(struct procgl_texture* tex)
{
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

static unsigned procgl_texture_height(struct procgl_texture* tex, int x, int y)
{
    if(x < 0) x = tex->w - (-x) % tex->w;
    else if(x >= tex->w) x = x % tex->w;
    if(y < 0) y = tex->h - (-y) % tex->h;
    else if(y >= tex->h) y = y % tex->h;
    return tex->normals[x + y * tex->w].h;
}

void procgl_texture_generate_normals(struct procgl_texture* tex)
{
    int x, y;
    for(x = 0; x < tex->w; ++x) {
        for(y = 0; y < tex->h; ++y) {
            float u, d, r, l;
            u = (float)procgl_texture_height(tex, x, y - 1) / 256;
            d = (float)procgl_texture_height(tex, x, y + 1) / 256;
            l = (float)procgl_texture_height(tex, x - 1, y) / 256;
            r = (float)procgl_texture_height(tex, x + 1, y) / 256;
            vec3 normal = { r - l, u - d, 2.0f };
            vec3_normalize(normal, normal);
            tex->normals[x + y * tex->w] = (struct procgl_texture_normal) {
                (normal[0] + 1) / 2 * 256,
                (normal[1] + 1) / 2 * 256,
                (normal[2] + 1) / 2 * 128 + 127,
                tex->normals[x + y * tex->w].h };
        }
    }
}

#include <stdlib.h>
#include <math.h>
#include "ext/linmath.h"
#include "wave.h"
#include "heightmap.h"

void pg_heightmap_init(struct pg_heightmap* hmap, int w, int h)
{
    hmap->map = calloc(w * h, sizeof(float));
    hmap->w = w;
    hmap->h = h;
}

void pg_heightmap_deinit(struct pg_heightmap* hmap)
{
    free(hmap->map);
}

void pg_heightmap_from_wave(struct pg_heightmap* hmap, struct pg_wave* wave,
                            float fx, float fy)
{
    int x, y;
    for(x = 0; x < hmap->w; ++x) {
        for(y = 0; y < hmap->h; ++y) {
            vec2 p = { (float)x / hmap->w * fx, (float)y / hmap->h * fy };
            hmap->map[x + y * hmap->w] = pg_wave_sample(wave, 2, p);
        }
    }
}

float pg_heightmap_get_height(struct pg_heightmap* hmap, int x, int y)
{
    if(x < 0 || x >= hmap->w) x = MOD(x, hmap->w);
    if(y < 0 || y >= hmap->h) y = MOD(y, hmap->h);
    return hmap->map[x + y * hmap->w];
}

float pg_heightmap_get_height_lerp(struct pg_heightmap* hmap, float x, float y)
{
    if(x < 0 || x >= hmap->w || y < 0 || y >= hmap->h) return 0;

    float xf, yf, xi, yi;
    xf = modff(x, &xi);
    yf = modff(y, &yi);
    float tl = pg_heightmap_get_height(hmap, xi, yi);
    float tr = pg_heightmap_get_height(hmap, xi + 1, yi);
    float bl = pg_heightmap_get_height(hmap, xi, yi + 1);
    float br = pg_heightmap_get_height(hmap, xi + 1, yi + 1);
    float l0 = tl + xf * (tr - tl);
    float l1 = bl + xf * (br - bl);
    return l0 + yf * (l1 - l0);
}

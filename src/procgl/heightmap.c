#include <stdlib.h>
#include <math.h>
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
            hmap->map[x + y * hmap->w] =
                pg_wave_sample(wave, 2, x * fx, y * fy);
        }
    }
}

float pg_heightmap_get_height(struct pg_heightmap* hmap, float x, float y)
{
    if(x < 0 || x >= hmap->w || y < 0 || y >= hmap->h) return 0;
    float xf, yf, xi, yi;
    xf = modff(x, &xi);
    yf = modff(y, &yi);
    float tl = hmap->map[(int)xi + (int)yi * hmap->w];
    float tr = hmap->map[(int)(xi + 1) + (int)yi * hmap->w];
    float bl = hmap->map[(int)xi + (int)(yi + 1) * hmap->w];
    float br = hmap->map[(int)(xi + 1) + (int)(yi + 1) * hmap->w];
    float l0 = tl + xf * (tr - tl);
    float l1 = bl + xf * (br - bl);
    return l0 + yf * (l1 - l0);
}

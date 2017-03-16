struct pg_heightmap {
    float* map;
    int w, h;
};

void pg_heightmap_init(struct pg_heightmap* hmap, int w, int h);
void pg_heightmap_deinit(struct pg_heightmap* hmap);
void pg_heightmap_from_wave(struct pg_heightmap* hmap, struct pg_wave* wave,
                            float fx, float fy);
float pg_heightmap_get_height(struct pg_heightmap* hmap, float x, float y);

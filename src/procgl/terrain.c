#include "renderer.h"
#include "texture.h"
#include "model.h"
#include "wave.h"
#include "terrain.h"
#include "noise1234.h"

static void pg_terrain_update_tris(struct pg_terrain* terr)
{
    unsigned* tris = terr->model.tris.data;
    int x, y, tri = 0;
    for(x = 0; x < terr->w - 1; ++x) {
        for(y = 0; y < terr->h - 1; ++y) {
                tris[tri++] = x + y * terr->w;
                tris[tri++] = (x + 1) + y * terr->w;
                tris[tri++] = x + (y + 1) * terr->w;
                tris[tri++] = (x + 1) + (y + 1) * terr->w;
                tris[tri++] = x + (y + 1) * terr->w;
                tris[tri++] = (x + 1) + y * terr->w;
        }
    }
}

void pg_terrain_init(struct pg_terrain* terr, int w, int h,
                         float precision)
{
    pg_model_init(&terr->model);
    pg_model_reserve_verts(&terr->model, w * h);
    pg_model_reserve_tris(&terr->model, (h - 1) * (h - 1) * 2);
    terr->heightmap = calloc(w * h, sizeof(float));
    terr->w = w;
    terr->h = h;
    terr->precision = precision;
    pg_terrain_update_tris(terr);
}

void pg_terrain_deinit(struct pg_terrain* terr)
{
    pg_model_deinit(&terr->model);
    free(terr->heightmap);
}

void pg_terrain_begin(struct pg_terrain* terr,
                          struct pg_renderer* rend)
{
    glBindVertexArray(terr->model.vao);
}

void pg_terrain_buffer(struct pg_terrain* terr, struct pg_renderer* rend)
{
    glBindVertexArray(terr->model.vao);
    glBindBuffer(GL_ARRAY_BUFFER, terr->model.verts_gl);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terr->model.tris_gl);
    glBufferData(GL_ARRAY_BUFFER, terr->model.verts.len * sizeof(struct pg_vert3d),
                 terr->model.verts.data, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terr->model.tris.len * sizeof(unsigned),
                 terr->model.tris.data, GL_STATIC_DRAW);
    glVertexAttribPointer(rend->shader_terrain.attrs.pos, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), 0);
    glVertexAttribPointer(rend->shader_terrain.attrs.normal, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(rend->shader_terrain.attrs.tangent, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(6 * sizeof(float)));
    glVertexAttribPointer(rend->shader_terrain.attrs.bitangent, 3, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(9 * sizeof(float)));
    glVertexAttribPointer(rend->shader_terrain.attrs.tex_coord, 2, GL_FLOAT, GL_FALSE,
                          14 * sizeof(float), (void*)(12 * sizeof(float)));
    glEnableVertexAttribArray(rend->shader_terrain.attrs.pos);
    glEnableVertexAttribArray(rend->shader_terrain.attrs.normal);
    glEnableVertexAttribArray(rend->shader_terrain.attrs.tangent);
    glEnableVertexAttribArray(rend->shader_terrain.attrs.bitangent);
    glEnableVertexAttribArray(rend->shader_terrain.attrs.tex_coord);
}

void pg_terrain_update(struct pg_terrain* terr)
{
    vec3 tangent = { 1, 0, 0 };
    struct pg_vert3d* verts = terr->model.verts.data;
    int x, y;
    for(x = 0; x < terr->w; ++x) {
        for(y = 0; y < terr->h; ++y) {
            vec3 norm;
            pg_terrain_normal(terr, x, y, norm);
            verts[x + y * terr->w] = (struct pg_vert3d) {
                .pos = { (((float)x / terr->w) * 2 - 1) * 4,
                         (((float)y / terr->h) * 2 - 1) * 4,
                         terr->heightmap[x + y * terr->w] / 4 },
                .tex_coord = { x, y } };
            //vec3_set(verts[x + y * terr->w].normal, norm[0], norm[1], norm[2]);
            //vec3_mul_cross(verts[x + y * terr->w].tangent, verts[x + y * terr->w].normal, tangent);
        }
    }
}

void pg_terrain_draw(struct pg_renderer* rend,
                         struct pg_terrain* terr, mat4 transform)
{
    glUniformMatrix4fv(rend->shader_terrain.model_matrix, 1, GL_FALSE, *transform);
    glDrawElements(GL_TRIANGLES, terr->model.tris.len, GL_UNSIGNED_INT, 0);
}

void pg_terrain_from_wave(struct pg_terrain* terr, struct pg_wave* wave,
                              vec3 domain)
{
    int x, y;
    for(x = 0; x < terr->w; ++x) {
        for(y = 0; y < terr-> h; ++y) {
            float x_ = (((float)x / terr->w) * 2 - 1) * domain[0];
            float y_ = (((float)y / terr->h) * 2 - 1) * domain[1];
            terr->heightmap[x + y * terr->w] =
                pg_wave_sample(wave, 2, x_, y_) * domain[2];
        }
    }
    pg_terrain_update(terr);
}

void pg_terrain_perlin(struct pg_terrain* terr, float scale,
                           float x1, float y1, float x2, float y2)
{
    int x, y;
    for(x = 0; x < terr->w; ++x) {
        for(y = 0; y < terr->h; ++y) {
            float x_ = (float)x / terr->w;
            float y_ = (float)y / terr->h;
            float dx = x2 - x1;
            float dy = y2 - y1;
            float nx = x1 + cos(x_ * 2 * M_PI) * dx / (2 * M_PI);
            float ny = y1 + cos(y_ * 2 * M_PI) * dy / (2 * M_PI);
            float nz = x1 + sin(x_ * 2 * M_PI) * dx / (2 * M_PI);
            float nw = y1 + sin(y_ * 2 * M_PI) * dy / (2 * M_PI);
            float noise = noise4(nx, ny, nz, nw);
            terr->heightmap[x + y * terr->w] = (noise * 0.5 + 0.5) * scale;
        }
    }
    pg_terrain_update(terr);
}

float pg_terrain_height(struct pg_terrain* terr, int x, int y)
{
    if(x < 0) x = 0;
    else if(x >= terr->w) x = terr->w - 1;
    if(y < 0) y = 0;
    else if(y >=  terr->h) y = terr->h - 1;
    return terr->heightmap[x + y * terr->w];
}

void pg_terrain_normal(struct pg_terrain* terr, int x, int y, vec3 out)
{
    float u, d, r, l;
    u = pg_terrain_height(terr, x, y - 1);
    d = pg_terrain_height(terr, x, y + 1);
    l = pg_terrain_height(terr, x - 1, y);
    r = pg_terrain_height(terr, x + 1, y);
    vec3 normal = { l - r, u - d, 2.0f };
    vec3_normalize(normal, normal);
    out[0] = normal[0];
    out[1] = normal[1];
    out[2] = normal[2];
}


#include "../linmath.h"
#include "../procgl/procgl.h"
#include "../shaders/shaders.h"
#include "game.h"
#define GAME_WIDTH 16
#define GAME_RADIUS 160
#define GAME_SEGMENTS 30

static inline float randf(void)
{
    return rand() / (float)RAND_MAX;
}

static void collider_generate_ring_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128, 0, 1);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            float _x = x - 64;
            float _y = y - 64;
            float dist = sqrt(_x * _x + _y * _y) - 40;
            if(dist < 0 || dist >= 24) {
                tex->pixels[x + y * tex->w] =
                    (struct pg_texture_pixel){ 50, 50, 100, 0 };
                continue;
            }
            dist -= 12;
            float r_dist = 1 - fabs(dist) / 12;
            uint8_t c = r_dist * 128 + 100;
            tex->pixels[x + y * tex->w] =
                (struct pg_texture_pixel) { c * 0.5, c * 0.5, c, 255 };
            tex->normals[x + y * tex->w].h = r_dist * 255;
        }
    }
    pg_texture_generate_normals(tex);
    pg_texture_buffer(tex);
}

static void collider_generate_lead_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128, 4, 5);
    int i;
    for(i = 0; i < 12; ++i) {
        int neutron = (i % 2);
        float angle, distance;
        if(i >= 4) {
            angle = (M_PI * 2) / 8 * (i - 4);
            distance = 32;
        } else {
            angle = (M_PI * 2) / 4 * i + M_PI / 4;
            distance = 16;
        }
        int nucleon_x = 64 + cos(angle) * distance;
        int nucleon_y = 64 + sin(angle) * distance;
        int x, y;
        for(x = 0; x < 64; ++x) {
            for(y = 0; y < 64; ++y) {
                int pix_x = x + nucleon_x - 20;
                int pix_y = y + nucleon_y - 20;
                float dist = sqrt((x - 20) * (x - 20) + (y - 20) * (y - 20));
                float r_dist = 1 - fabs(dist) / 20;
                uint8_t c = r_dist * 62 + 150;
                if(dist <= 20
                && (pix_x >= 0 && pix_y >= 0 && pix_x < 128 && pix_y < 128)
                && tex->normals[pix_x + pix_y * tex->w].h <= (r_dist * 255)) {
                    tex->pixels[pix_x + pix_y * tex->w] = neutron ?
                        (struct pg_texture_pixel){ c, c * 0.5, c * 0.5, 255 } :
                        (struct pg_texture_pixel){ c * 0.5, c * 0.5, c, 255 };
                    tex->normals[pix_x + pix_y * tex->w].h = r_dist * 255;
                }
            }
        }
    }
    pg_texture_generate_normals(tex);
    pg_texture_buffer(tex);
}

static void collider_generate_ring_model(struct pg_model* model,
                                         struct pg_shader* shader)
{
    pg_model_quad(model);
    pg_model_precalc_verts(model);
    pg_model_buffer(model, shader);
}

static void collider_generate_env_texture(struct pg_texture* tex)
{
    pg_texture_init(tex, 128, 128, 2, 3);
    int x, y;
    for(x = 0; x < 128; ++x) {
        for(y = 0; y < 128; ++y) {
            struct pg_texture_pixel* p = &tex->pixels[x + y * tex->w];
            struct pg_texture_normal* n = &tex->normals[x + y * tex->w];
            float _x = x - 64;
            float _y = y - 64;
            float dist = sqrt(_x * _x + _y * _y);
            if(dist >= 50) {
                float s = fabs(sin(y / (128 / (M_PI * 2)) * 8));
                s = abs((y % 8) - 4);
                float c = s * 16 + 64;
                *p = (struct pg_texture_pixel){ 128, 128, 128, 255 };
                n->h = pow(s, 3) * 3;
            } else if(dist >= 35) {
                float s = pow(8 - fabs(dist - 43), 2) + 60;
                *p = (struct pg_texture_pixel){ s * 2, s * 2, s * 2, 255 };
                n->h = 255 - (s * 2);
            } else {
                float angle = atan2(x - 64, y - 64) + M_PI;
                float angle_s = fmod(angle, (M_PI * 2) / 8) / ((M_PI * 2) / 8);
                float c = fabs((1 - angle_s) * 8 - 4) * 60;
                *p = (struct pg_texture_pixel){ c, c, c < 100 ? c * 2 : c, 255 };
                n->h = c * 230;
            }
        }
    }
    pg_texture_generate_normals(tex);
    pg_texture_buffer(tex);
}

static void collider_generate_env_model(struct pg_model* model,
                                        struct pg_shader* shader)
{
    float half_angle = M_PI / GAME_SEGMENTS;
    pg_model_init(model);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 0, 1, 2);
    pg_model_add_triangle(model, 2, 1, 3);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 5, 4, 6);
    pg_model_add_triangle(model, 5, 6, 7);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), -GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 9, 8, 10);
    pg_model_add_triangle(model, 9, 10, 11);
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 0, 1 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS - GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS - GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 0 } });
    pg_model_add_vertex(model, &(struct pg_vert3d) {
        .pos = { (GAME_RADIUS + GAME_WIDTH / 2) * cos(-half_angle),
                 (GAME_RADIUS + GAME_WIDTH / 2) * sin(-half_angle), GAME_WIDTH / 2 },
        .tex_coord = { 1, 1 } });
    pg_model_add_triangle(model, 12, 13, 14);
    pg_model_add_triangle(model, 14, 13, 15);
    pg_model_precalc_verts(model);
    pg_model_buffer(model, shader);
}

void collider_init(struct collider_state* coll)
{
    int sw, sh;
    pg_screen_size(&sw, &sh);
    pg_gbuffer_init(&coll->gbuf, sw, sh);
    pg_gbuffer_bind(&coll->gbuf, 16, 17, 18, 19);
    collider_generate_ring_texture(&coll->ring_texture);
    collider_generate_env_texture(&coll->env_texture);
    collider_generate_lead_texture(&coll->lead_texture);
    pg_texture_init_from_file(&coll->font, "font_8x8.png", NULL, 8, -1);
    pg_texture_set_atlas(&coll->font, 8, 8);
    pg_shader_text(&coll->shader_text);
    pg_shader_text_set_font(&coll->shader_text, &coll->font);
    pg_shader_3d(&coll->shader_3d);
    collider_generate_ring_model(&coll->ring_model, &coll->shader_3d);
    collider_generate_env_model(&coll->env_model, &coll->shader_3d);
    coll->player_angle = 0.3;
    coll->player_speed = 0.1;
    coll->player_light_intensity = 10;
    vec2_set(coll->player_pos, 0, 0);
    pg_viewer_init(&coll->view, (vec3){ 0, 0, 0 }, (vec2){ 0, 0 },
                   (vec2){ sw, sh }, (vec2){ 0.1, 200 });
    coll->curl = curl_easy_init();
    coll->state = LHC_MENU;
}

void collider_deinit(struct collider_state* coll)
{
    pg_shader_deinit(&coll->shader_3d);
    pg_model_deinit(&coll->ring_model);
    pg_texture_deinit(&coll->ring_texture);
    ARR_DEINIT(coll->rings);
}

static inline float ring_power_func(void)
{
    float n = randf();
    return pow((n - 0.1), 3) + 0.1;
}

static void collider_generate_ring(struct collider_state* coll)
{
    float limit = GAME_WIDTH * 0.5;
    switch(coll->ring_generator) {
    default:
    case RING_RANDOM: {
        struct coll_ring r;
        vec2_set(r.pos, randf() * limit - limit / 2,
                        randf() * limit - limit / 2);
        r.angle = coll->last_ring.angle + coll->ring_distance;
        r.angle = fmod(r.angle, M_PI * 2);
        r.power = ring_power_func();
        ARR_PUSH(coll->rings, r);
        coll->last_ring = r;
        break;
    }
    }
}

/*  This function should approach 1 as x approaches infinity
    1 here represents the speed of light    */
static float speed_func(float x)
{
    return (x / (x + 1)) * 0.03;
}
static float speed_func_display(float x)
{
    x *= 5;
    return (x / (x + 1));
}

static void collider_update_gameplay(struct collider_state* coll)
{
    int mouse_x, mouse_y;
    float delta_time = pg_delta_time(0);
    SDL_GetRelativeMouseState(&mouse_x, &mouse_y);
    coll->player_pos[0] += mouse_x * 0.005;
    coll->player_pos[1] -= mouse_y * 0.005;
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT) coll->state = LHC_EXIT;
    }
    float pos_dist = vec2_len(coll->player_pos);
    if(pos_dist > GAME_WIDTH / 2 - 1.5) {
        vec2_normalize(coll->player_pos, coll->player_pos);
        vec2_scale(coll->player_pos, coll->player_pos, GAME_WIDTH / 2 - 1.5);
    }
    pg_viewer_set(&coll->view,
        (vec3){ (GAME_RADIUS + coll->player_pos[0]) * cos(coll->player_angle),
                (GAME_RADIUS + coll->player_pos[0]) * sin(coll->player_angle),
                coll->player_pos[1] },
        (vec2){ coll->player_angle + M_PI / 2, 0 });
    float new_angle = coll->player_angle +
        (speed_func(coll->player_speed) * 60 * delta_time);
    struct coll_ring* r;
    int i;
    ARR_FOREACH_PTR_REV(coll->rings, r, i) {
        if((new_angle >= M_PI * 2 && fmod(new_angle, M_PI * 2) >= r->angle)
        || (coll->player_angle < r->angle && new_angle >= r->angle)) {
            vec2 diff;
            vec2_sub(diff, coll->player_pos, r->pos);
            float dist = vec2_len(diff);
            float radius = ((1 - r->power) * 8) * 0.35;
            if(dist < radius) {
                coll->player_speed += r->power * 0.1;
                coll->player_light_intensity = 1;
            }
            ARR_SWAPSPLICE(coll->rings, i, 1);
            collider_generate_ring(coll);
        }
    }
    float new_lead_angle = coll->lead_angle -
        (speed_func(coll->lead_speed) * 60 * delta_time);
    if(coll->player_angle < coll->lead_angle && new_angle >= new_lead_angle) {
        vec2 diff;
        vec2_sub(diff, coll->player_pos, coll->lead_pos);
        float dist = vec2_len(diff);
        if(dist <= 2.5) {
            SDL_StartTextInput();
            coll->lhc_fun_fact = rand() % 10;
            coll->state = LHC_GAMEOVER;
            coll->lead_angle = coll->player_angle;
        } else {
            float limit = GAME_WIDTH * 0.7;
            coll->lead_angle = coll->player_angle;
            vec2_set(coll->lead_pos,
                randf() * limit - limit / 2, randf() * limit - limit / 2);
        }
    }
    coll->player_angle = fmod(new_angle, M_PI * 2);
    coll->player_light_intensity -=
        coll->player_light_intensity * (0.60 * delta_time);
    new_angle = coll->lead_angle -
        speed_func(coll->lead_speed) * 60 * delta_time;
    if(new_angle < 0) new_angle = (M_PI * 2) - new_angle;
    coll->lead_angle = new_angle;
    coll->lead_speed = coll->player_speed;
}

char upload_result[16];
static size_t curl_cb(char* ptr, size_t size, size_t nmemb, void* udata)
{
    strncpy(upload_result, ptr, 16);
}

#include "elympics_key.h"
static void collider_upload_score(struct collider_state* coll)
{
    char url[256] = {};
    snprintf(url, 256, "https://dollarone.games/elympics/submitHighscore?key="
             ELYMPICS_KEY "&name=%s&score=%.5f",
             coll->playername_idx > 0 ? coll->playername : "CERN",
             speed_func_display(coll->player_speed));
    curl_easy_setopt(coll->curl, CURLOPT_URL, url);
    curl_easy_setopt(coll->curl, CURLOPT_WRITEFUNCTION, curl_cb);
    CURLcode res = curl_easy_perform(coll->curl);
    if(res != CURLE_OK || strncmp(upload_result, "OK", 16) != 0) {
        coll->score_upload_state = LHC_UPLOADED_SCORE;
    } else {
        coll->score_upload_state = LHC_UPLOAD_FAILED;
    }
}

static void collider_reset_game(struct collider_state* coll)
{
    ARR_TRUNCATE(coll->rings, 0);
    coll->player_angle = 0.3;
    coll->player_speed = 0.1;
    coll->player_light_intensity = 10;
    vec2_set(coll->player_pos, 0, 0);
}

static void collider_update_gameover(struct collider_state* coll)
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT) coll->state = LHC_EXIT;
        else if(e.type == SDL_KEYDOWN) {
            if(coll->score_upload_state != LHC_UPLOADED_SCORE) {
                if(e.key.keysym.sym == SDLK_BACKSPACE
                && coll->playername_idx > 0) {
                    coll->playername[--coll->playername_idx] = 0;
                } else if(e.key.keysym.sym == SDLK_RETURN) {
                    collider_upload_score(coll);
                }
            }
            if(e.key.keysym.sym == SDLK_ESCAPE) {
                coll->state = LHC_MENU;
                collider_reset_game(coll);
                return;
            }
        } else if(e.type == SDL_TEXTINPUT
               && coll->score_upload_state != LHC_UPLOADED_SCORE) {
            if(coll->playername_idx < 31 && e.text.text[0] < 0x80) {
                coll->playername[coll->playername_idx++] = e.text.text[0];
            }
        }
    }
    float angle = coll->player_angle - 0.2;
    pg_viewer_set(&coll->view,
        (vec3){ (GAME_RADIUS) * cos(angle), (GAME_RADIUS) * sin(angle), 0 },
        (vec2){ angle + M_PI / 2, 0 });
}

static void collider_setup_game(struct collider_state* coll)
{
    ARR_INIT(coll->rings);
    int i;
    for(i = 1; i < 6; ++i) {
        struct coll_ring r =
            { 0.1, (M_PI) / 6 * i, { (i - 6) * 0.25, (i - 6) * 0.25 } };
        ARR_PUSH(coll->rings, r);
    }
    coll->ring_generator = RING_RANDOM;
    coll->ring_distance = 1;
    coll->last_ring = coll->rings.data[coll->rings.len - 1];
    coll->player_angle = 0.3;
    coll->player_speed = 0.1;
    coll->player_light_intensity = 2;
    vec2_set(coll->player_pos, 0, 0);
    coll->lead_angle = 0;
    coll->lead_speed = 0;
    vec2_set(coll->lead_pos, 0, 0);
    coll->score_upload_state = LHC_NOT_UPLOADED;
    memset(coll->playername, 0, 32);
    coll->playername_idx = 0;
}

static void collider_update_menu(struct collider_state* coll)
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
        if(e.type == SDL_QUIT) coll->state = LHC_EXIT;
        else if(e.type == SDL_KEYDOWN) {
            if(e.key.keysym.sym == SDLK_SPACE) {
                coll->state = LHC_PLAY;
                collider_setup_game(coll);
                return;
            } else if(e.key.keysym.sym == SDLK_ESCAPE) {
                coll->state = LHC_EXIT;
            }
        }
    }
    float pos_dist = vec2_len(coll->player_pos);
    if(pos_dist > GAME_WIDTH / 2 - 1.5) {
        vec2_normalize(coll->player_pos, coll->player_pos);
        vec2_scale(coll->player_pos, coll->player_pos, GAME_WIDTH / 2 - 1.5);
    }
    pg_viewer_set(&coll->view,
        (vec3){ (GAME_RADIUS + coll->player_pos[0]) * cos(coll->player_angle),
                (GAME_RADIUS + coll->player_pos[0]) * sin(coll->player_angle),
                coll->player_pos[1] },
        (vec2){ coll->player_angle + M_PI / 2, 0 });
    float delta_time = pg_delta_time(0);
    float new_angle = coll->player_angle +
        (speed_func(coll->player_speed) * 60 * delta_time);
    coll->player_angle = fmod(new_angle, M_PI * 2);
}

void collider_update(struct collider_state* coll)
{
    switch(coll->state) {
    case LHC_MENU: collider_update_menu(coll); break;
    case LHC_PLAY: collider_update_gameplay(coll); break;
    case LHC_GAMEOVER: collider_update_gameover(coll); break;
    }
}

static void collider_draw_gameplay(struct collider_state* coll)
{
    pg_gbuffer_dst(&coll->gbuf);
    /*  All of this renders to the gbuffer, for lighting later  */
    pg_shader_begin(&coll->shader_3d, &coll->view);
    pg_shader_3d_set_texture(&coll->shader_3d, 2, 3);
    pg_model_begin(&coll->env_model);
    int i;
    for(i = 0; i < GAME_SEGMENTS; ++i) {
        mat4 model_transform;
        mat4_identity(model_transform);
        mat4_rotate_Z(model_transform, model_transform, (M_PI * 2) / GAME_SEGMENTS * i);
        pg_model_draw(&coll->env_model, &coll->shader_3d, model_transform);
    }
    pg_shader_3d_set_texture(&coll->shader_3d, 0, 1);
    pg_model_begin(&coll->ring_model);
    struct coll_ring* r;
    ARR_FOREACH_PTR(coll->rings, r, i) {
        float radius = (1 - r->power) * 8;
        mat4 model_transform;
        mat4_translate(model_transform,
            (GAME_RADIUS + r->pos[0]) * cos(r->angle),
            (GAME_RADIUS + r->pos[0]) * sin(r->angle), r->pos[1]);
        mat4_rotate_Z(model_transform, model_transform, r->angle);
        mat4_scale_aniso(model_transform, model_transform, radius, radius, radius);
        pg_model_draw(&coll->ring_model, &coll->shader_3d, model_transform);
    }
    mat4 model_transform;
    mat4_translate(model_transform,
        (GAME_RADIUS + coll->lead_pos[0]) * cos(coll->lead_angle),
        (GAME_RADIUS + coll->lead_pos[0]) * sin(coll->lead_angle),
        coll->lead_pos[1]);
    mat4_rotate_Z(model_transform, model_transform, coll->lead_angle);
    mat4_scale_aniso(model_transform, model_transform, 6, 6, 6);
    pg_shader_3d_set_texture(&coll->shader_3d, 4, 5);
    pg_model_draw(&coll->ring_model, &coll->shader_3d, model_transform);
    /*  Now we start drawing all the lights */
    pg_gbuffer_begin_light(&coll->gbuf, &coll->view);
    float l = fmod(pg_time() * 0.005, GAME_SEGMENTS);
    for(i = 0; i < GAME_SEGMENTS; ++i) {
        float angle = (M_PI * 2) / GAME_SEGMENTS * i;
        angle += (M_PI * 2) / GAME_SEGMENTS / 2;
        float b = i - l > 3 ? 0 : (i - l < 0) ? 0 : (i - l) * 15;
        pg_gbuffer_draw_light(&coll->gbuf,
            (vec4){ (GAME_RADIUS) * cos(angle), (GAME_RADIUS) * sin(angle),
                    GAME_WIDTH / 2 - 0.3, 25 + b },
            (vec3){ 1, 1, 1 });
    }
    pg_gbuffer_draw_light(&coll->gbuf,
        (vec4){ GAME_RADIUS * cos(coll->lead_angle - 0.1),
                GAME_RADIUS * sin(coll->lead_angle - 0.1), 0, 30 },
        (vec3){ 1, 0.25, 0.25 });
    pg_gbuffer_draw_light(&coll->gbuf,
        (vec4){ coll->view.pos[0], coll->view.pos[1], coll->view.pos[2],
                coll->player_light_intensity * 50 },
        (vec3){ 0.5, 0.5, 1.5 });
    /*  And finish directly to the screen, with a tiny bit of ambient light */
    pg_screen_dst();
    pg_gbuffer_finish(&coll->gbuf, (vec3){ 0.1, 0.1, 0.1 });
    pg_shader_begin(&coll->shader_text, &coll->view);
    char speed_str[10];
    snprintf(speed_str, 10, "%.5fc", speed_func_display(coll->player_speed));
    pg_shader_text_write(&coll->shader_text, speed_str,
        (vec2){ 50, 50 }, (vec2){ 24, 24 }, 0.25);
    char fps_str[10];
    snprintf(fps_str, 10, "FPS: %d", (int)pg_framerate());
    pg_shader_text_write(&coll->shader_text, fps_str,
        (vec2){ 0, 0 }, (vec2){ 8, 8 }, 0.125);
}

char* LHC_facts[10][3] = {
    {   "THE LARGE HADRON COLLIDER CAN ACCELERATE",
        "PROTONS UP TO 0.99999999C, WITHIN WALKING",
        "DISTANCE OF LIGHT SPEED!" },
    {   "THE LARGE HADRON COLLIDER SITS INSIDE A",
        "CIRCULAR TUNNEL WHICH IS ABOUT 27KM (~17MI)",
        "LONG!" },
    {   "THE LARGE HADRON COLLIDER IS THE LARGEST",
        "MACHINE EVER BUILT BY HUMANS!" },
    {   "TWO NOBEL PRIZES HAVE BEEN AWARDED TO",
        "SCIENTISTS WHOSE EXPERIMENTS WERE CONDUCTED",
        "AT CERN!" },
    {   "ALL OF THE SUPER-CONDUCTING STRANDS IN THE",
        "LARGE HADRON COLLIDER WOULD STRETCH AROUND",
        "EARTH'S EQUATER NEARLY SEVEN TIMES!" },
    {   "THE WORLDWIDE LHC COMPUTING GRID IS THE",
        "WORLD'S LARGEST DISTRIBUTED COMPUTING GRID",
        "WITH OVER 170 FACILITIES IN 36 COUNTRIES!" },
    {   "THE LARGE HADRON COLLIDER PRODUCES OVER",
        "15 PETABYTES OF SCIENTIFIC DATA PER YEAR!" },
    {   "IN MAY 2011, QUARK-GLUON PLASMA, WHICH IS",
        "THE DENSEST MATTER THOUGHT TO EXIST BESIDES",
        "BLACK HOLES, WAS CREATED AT THE LHC!" },
    {   "IT TAKES A PROTON LESS THAN 90 MICROSECONDS",
        "TO PASS AROUND THE MAIN RING OF THE LARGE",
        "HADRON COLLIDER!" },
    {   "THE LARGE HADRON COLLIDER CAN DETECT ABOUT",
        "FORTY MILLION COLLISIONS PER SECOND!" }
};

static void collider_draw_gameover(struct collider_state* coll)
{
    pg_gbuffer_dst(&coll->gbuf);
    /*  All of this renders to the gbuffer, for lighting later  */
    pg_shader_begin(&coll->shader_3d, &coll->view);
    pg_shader_3d_set_texture(&coll->shader_3d, 2, 3);
    pg_model_begin(&coll->env_model);
    int i;
    for(i = 0; i < GAME_SEGMENTS; ++i) {
        mat4 model_transform;
        mat4_identity(model_transform);
        mat4_rotate_Z(model_transform, model_transform, (M_PI * 2) / GAME_SEGMENTS * i);
        pg_model_draw(&coll->env_model, &coll->shader_3d, model_transform);
    }
    pg_shader_3d_set_texture(&coll->shader_3d, 0, 1);
    pg_model_begin(&coll->ring_model);
    struct coll_ring* r;
    ARR_FOREACH_PTR(coll->rings, r, i) {
        float radius = (1 - r->power) * 8;
        mat4 model_transform;
        mat4_translate(model_transform,
            (GAME_RADIUS + r->pos[0]) * cos(r->angle),
            (GAME_RADIUS + r->pos[0]) * sin(r->angle), r->pos[1]);
        mat4_rotate_Z(model_transform, model_transform, r->angle);
        mat4_scale_aniso(model_transform, model_transform, radius, radius, radius);
        pg_model_draw(&coll->ring_model, &coll->shader_3d, model_transform);
    }
    mat4 model_transform;
    mat4_translate(model_transform,
        (GAME_RADIUS + coll->lead_pos[0]) * cos(coll->lead_angle),
        (GAME_RADIUS + coll->lead_pos[0]) * sin(coll->lead_angle),
        coll->lead_pos[1]);
    mat4_rotate_Z(model_transform, model_transform, coll->lead_angle);
    mat4_scale_aniso(model_transform, model_transform, 6, 6, 6);
    pg_shader_3d_set_texture(&coll->shader_3d, 4, 5);
    pg_model_draw(&coll->ring_model, &coll->shader_3d, model_transform);
    /*  Now we start drawing all the lights */
    pg_gbuffer_begin_light(&coll->gbuf, &coll->view);
    pg_gbuffer_draw_light(&coll->gbuf,
        (vec4){ GAME_RADIUS * cos(coll->lead_angle - 0.05),
                GAME_RADIUS * sin(coll->lead_angle - 0.05), GAME_WIDTH / 2 - 1,
                25 + sin(pg_time() * 0.025) * 2 },
        (vec3){ 0.5, 1.5, 0.5 });
    /*  And finish directly to the screen, with a tiny bit of ambient light */
    pg_screen_dst();
    pg_gbuffer_finish(&coll->gbuf, (vec3){ 0.1, 0.1, 0.1 });
    pg_shader_begin(&coll->shader_text, &coll->view);
    char speed_str[32];
    snprintf(speed_str, 32, "YOU REACHED %.5fc!", speed_func_display(coll->player_speed));
    pg_shader_text_write(&coll->shader_text, speed_str,
        (vec2){ 50, 50 }, (vec2){ 24, 24 }, 0.25);
    pg_shader_text_write(&coll->shader_text, "GIVE US YOUR NAME FOR THE NOBEL PRIZE:",
        (vec2){ 100, 100 }, (vec2){ 16, 16 }, 0.1);
    pg_shader_text_write(&coll->shader_text, coll->playername,
        (vec2){ 140, 140 }, (vec2){ 24, 24 }, 0.25);
    pg_shader_text_write(&coll->shader_text, "PRESS ESC TO RETURN TO THE MAIN MENU",
        (vec2){ 100, 250 }, (vec2){ 16, 16 }, 0.1);
    switch(coll->score_upload_state) {
    case LHC_NOT_UPLOADED:
        pg_shader_text_write(&coll->shader_text, "PRESS ENTER TO UPLOAD THE SCIENCE!",
            (vec2){ 100, 200 }, (vec2){ 16, 16 }, 0.1);
        break;
    case LHC_UPLOADED_SCORE:
        pg_shader_text_write(&coll->shader_text, "YOUR NOBEL PRIZE IS IN THE MAIL",
            (vec2){ 100, 200 }, (vec2){ 16, 16 }, 0.1);
        break;
    case LHC_UPLOAD_FAILED:
        pg_shader_text_write(&coll->shader_text, "NET DOWN DUE TO BLACK HOLE DISASTER?",
            (vec2){ 50, 200 }, (vec2){ 16, 16 }, 0.1);
        break;
    }
    pg_shader_text_write(&coll->shader_text, "DID YOU KNOW?",
        (vec2){ 160, 360 }, (vec2){ 24, 24 }, 0.25);
    for(i = 0; i < 3; ++i) {
        if(LHC_facts[coll->lhc_fun_fact][i]) {
            pg_shader_text_write(&coll->shader_text, LHC_facts[coll->lhc_fun_fact][i],
                (vec2){ 50, 400 + i * 24 }, (vec2){ 16, 16 }, 0.0625);
        }
    }
}

static void collider_draw_menu(struct collider_state* coll)
{
    pg_gbuffer_dst(&coll->gbuf);
    /*  All of this renders to the gbuffer, for lighting later  */
    pg_shader_begin(&coll->shader_3d, &coll->view);
    pg_shader_3d_set_texture(&coll->shader_3d, 2, 3);
    pg_model_begin(&coll->env_model);
    int i;
    for(i = 0; i < GAME_SEGMENTS; ++i) {
        mat4 model_transform;
        mat4_identity(model_transform);
        mat4_rotate_Z(model_transform, model_transform, (M_PI * 2) / GAME_SEGMENTS * i);
        pg_model_draw(&coll->env_model, &coll->shader_3d, model_transform);
    }
    pg_gbuffer_begin_light(&coll->gbuf, &coll->view);
    float l = fmod(pg_time() * 0.005, GAME_SEGMENTS);
    for(i = 0; i < GAME_SEGMENTS; ++i) {
        float angle = (M_PI * 2) / GAME_SEGMENTS * i;
        angle += (M_PI * 2) / GAME_SEGMENTS / 2;
        float b = i - l > 3 ? 0 : (i - l < 0) ? 0 : (i - l) * 15;
        pg_gbuffer_draw_light(&coll->gbuf,
            (vec4){ (GAME_RADIUS) * cos(angle), (GAME_RADIUS) * sin(angle),
                    GAME_WIDTH / 2 - 0.3, 25 + b },
            (vec3){ 1, 1, 1 });
    }
    pg_gbuffer_draw_light(&coll->gbuf,
        (vec4){ coll->view.pos[0], coll->view.pos[1], coll->view.pos[2],
                50 + sin(pg_time() * 0.025) * 2 },
        (vec3){ 0.5, 0.5, 1.5 });
    pg_screen_dst();
    pg_gbuffer_finish(&coll->gbuf, (vec3){ 0.1, 0.1, 0.1 });
    pg_shader_begin(&coll->shader_text, &coll->view);
    int sw, sh;
    pg_screen_size(&sw, &sh);
    pg_shader_text_write(&coll->shader_text, "LUDUM",
        (vec2){ sw / 2 - 40 * 4.5, sh / 6}, (vec2){ 40, 40 }, 1);
    pg_shader_text_write(&coll->shader_text, "HADRON",
        (vec2){ sw / 2 - 40 * 5.5, sh / 6 * 2 }, (vec2){ 40, 40 }, 1);
    pg_shader_text_write(&coll->shader_text, "COLLIDER",
        (vec2){ sw / 2 - 40 * 7.5, sh / 6 * 3 }, (vec2){ 40, 40 }, 1);
    pg_shader_text_write(&coll->shader_text, "PRESS SPACE TO START",
        (vec2){ sw / 2 - 16 * 10, sh / 6 * 4 }, (vec2){ 16, 16 }, 0.1);
    pg_shader_text_write(&coll->shader_text, "OR ESCAPE TO EXIT",
        (vec2){ sw / 2 - 16 * 9 + 16, sh / 6 * 4 + 32 }, (vec2){ 16, 16 }, 0.1);
}

void collider_draw(struct collider_state* coll)
{
    switch(coll->state) {
    case LHC_MENU: collider_draw_menu(coll); break;
    case LHC_PLAY: collider_draw_gameplay(coll); break;
    case LHC_GAMEOVER: collider_draw_gameover(coll); break;
    }
}

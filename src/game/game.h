#define CURL_STATICLIB
#include "../libcurl/curl.h"
#include "../arr.h"

struct coll_ring {
    float power;
    float angle;
    vec2 pos;
};

struct collider_state {
    float mouse_sens;
    enum {
        LHC_EXIT,
        LHC_MENU,
        LHC_PLAY,
        LHC_GAMEOVER
    } state;
    int lhc_fun_fact;
    int playername_idx;
    char playername[32];
    struct pg_shader shader_3d;
    struct pg_shader shader_text;
    struct pg_texture font;
    struct pg_texture ring_texture;
    struct pg_model ring_model;
    struct pg_texture lead_texture;
    struct pg_texture env_texture;
    struct pg_model env_model;
    struct pg_viewer view;
    struct pg_gbuffer gbuf;
    float lead_angle;
    float lead_speed;
    vec2 lead_pos;
    float player_angle;
    float player_speed;
    float player_light_intensity;
    vec2 player_pos;
    enum {
        RING_RANDOM,
        RING_SERIES,
        RING_LINEAR,
        RING_SPIRAL
    } ring_generator;
    float ring_spiral_angle;
    vec2 ring_linear_dir;
    float ring_distance;
    struct coll_ring last_ring;
    ARR_T(struct coll_ring) rings;
    CURL* curl;
    enum {
        LHC_NOT_UPLOADED,
        LHC_UPLOADED_SCORE,
        LHC_UPLOAD_FAILED
    } score_upload_state;
};

void collider_init(struct collider_state* coll, float mouse_sens);
void collider_deinit(struct collider_state* coll);
void collider_update(struct collider_state* coll);
void collider_draw(struct collider_state* coll);

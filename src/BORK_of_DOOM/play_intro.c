#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "procgl/procgl.h"
#include "bork.h"
#include "particle.h"
#include "entity.h"
#include "map_area.h"
#include "bullet.h"
#include "physics.h"
#include "upgrades.h"
#include "recycler.h"
#include "game_states.h"
#include "state_play.h"

void tick_intro(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    --d->menu.intro.ticks;
    if(d->menu.intro.ticks <= 0) {
        d->menu.intro.ticks = PLAY_SECONDS(9);
        ++d->menu.intro.slide;
        if(d->menu.intro.slide == 10) {
            pg_audio_channel_pause(1, 0);
            pg_audio_channel_pause(2, 0);
            d->menu.state = BORK_MENU_CLOSED;
            pg_mouse_mode(1);
        } else if(d->menu.intro.slide >= 8) d->menu.intro.ticks = PLAY_SECONDS(4);
    }
    if(pg_check_input(kmap[BORK_CTRL_MENU], PG_CONTROL_HIT)
    || pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_MENU], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
        if(d->menu.intro.slide < 9) {
            d->menu.intro.slide = 9;
            d->menu.intro.ticks = PLAY_SECONDS(3);
        }
    }
}

static const struct intro_slide {
    int lines;
    char text[16][64];
} slides[] = {
{   .lines = 4,
    .text = {
"1903",
"KONSTANTIN TSIOLKOVSKY DERIVES",
"THE ROCKET EQUATION AND SHOWS",
"THAT SPACE TRAVEL IS POSSIBLE", } },
{   .lines = 2,
    .text = {
"1933",
"FIRST SOVIET ROCKET IS LAUNCHED", } },
{   .lines = 3,
    .text = {
"1951",
"FIRST OF SEVERAL SUB-ORBITAL",
"TEST FLIGHTS BY DOGS", } },
{   .lines = 4,
    .text = {
"THE DOGS IN THESE TEST FLIGHTS",
"EXHIBIT HEIGHTENED LEARNING",
"ABILITIES UPON THEIR RETURN TO",
"EARTH", } },
{   .lines = 4,
    .text = {
"OCTOBER 1957",
"THE FIRST ARTIFICIAL SATELLITE",
"SPUTNIK-1",
"ORBITS THE PLANET EARTH", } },
{   .lines = 3,
    .text = {
"NOVEMBER 1957",
"LAIKA IS THE FIRST DOG",
"LAUNCHED INTO ORBIT", } },
{   .lines = 3,
    .text = {
"UPON HER RETURN TO EARTH LAIKA",
"EXHIBITS VASTLY INCREASED",
"INTELLIGENCE", } },
{   .lines = 5,
    .text = {
"JANUARY 1958",
"THE SOVIET SPACE PROGRAM IS",
"REDIRECTED TOWARD STUDYING",
"THE EFFECTS OF SPACE ON",
"CANINE INTELLIGENCE", } },
{   .lines = 5,
    .text = {
"",
"",
"",
"",
"CANINE INTELLIGENCE", } } };


void draw_intro(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1.0f });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d, &d->core->shader_2d);
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){}, (vec2){ ar, 1 }, 0);
    pg_shader_2d_texture(&d->core->shader_2d, &d->core->radial_vignette);
    if(d->menu.intro.slide == 9) {
        float alpha = (float)d->menu.intro.ticks / PLAY_SECONDS(3);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 0, 0, 0, alpha }, (vec4){ 0, 0, 0, alpha });
    } else {
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){}, (vec4){ 0, 0, 0, 1 });
    }
    pg_model_draw(&d->core->quad_2d, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    if(d->menu.intro.slide >= 9) return;
    const struct intro_slide* slide = &slides[d->menu.intro.slide];
    float start_y = 0.5 - (slide->lines * 0.5) * 0.1;
    struct pg_shader_text text = { .use_blocks = slide->lines };
    float dist_from_middle = fabs(d->menu.intro.ticks - PLAY_SECONDS(4));
    dist_from_middle = MAX(PLAY_SECONDS(2), dist_from_middle);
    dist_from_middle = 1 - ((dist_from_middle - PLAY_SECONDS(2)) / PLAY_SECONDS(1));
    dist_from_middle = MAX(0, dist_from_middle);
    int i;
    for(i = 0; i < slide->lines; ++i) {
        int len = snprintf(text.block[i], 64, "%s", slide->text[i]);
        vec4_set(text.block_style[i], ar * 0.5 - (len * 0.03 * 1.25 * 0.5),
                                      start_y + i * 0.1, 0.03, 1.25);
        vec4_set(text.block_color[i], 1, 1, 1, dist_from_middle);
        if(((d->menu.intro.slide == 7 && d->menu.intro.ticks < PLAY_SECONDS(4)))
         && i == 4) text.block_color[i][3] = 1;
    }
    pg_shader_text_write(shader, &text);
}


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

void tick_outro(struct bork_play_data* d)
{
    uint8_t* kmap = d->core->ctrl_map;
    int8_t* gmap = d->core->gpad_map;
    ++d->menu.intro.ticks;
    if(d->menu.intro.ticks >= PLAY_SECONDS(9)) {
        d->menu.intro.ticks = 0;
        ++d->menu.intro.slide;
    }
    if(pg_check_input(kmap[BORK_CTRL_MENU], PG_CONTROL_HIT)
    || pg_check_input(kmap[BORK_CTRL_MENU_BACK], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_MENU], PG_CONTROL_HIT)
    || pg_check_gamepad(gmap[BORK_CTRL_SELECT], PG_CONTROL_HIT)) {
        if(d->menu.intro.slide < 5) {
            d->menu.intro.slide = 5;
            d->menu.intro.ticks = PLAY_SECONDS(3);
        } else {
            d->menu.intro.slide = 8;
            d->menu.intro.ticks = 0;
        }
    }
}

struct intro_slide {
    int lines;
    char text[16][64];
};

static const struct intro_slide slides[4][4] = {
/*  NO SCANNING, DID NOT KILL LAIKA */
{
{   .lines = 1,
    .text = {
"YOU HAVE ESCAPED THE DOGS!" } },
{   .lines = 4,
    .text = {
"ALTHOUGH LAIKA AND HER",
"COMRADES ARE STILL ALIVE,",
"YOU HAVE ESCAPED THE",
"SPACE STATION." } },
{   .lines = 3,
    .text = {
"WILL THEY GO ON TO TAKE",
"OVER THE WORLD BECAUSE",
"YOU SPARED THEM?" } },
{   .lines = 1,
    .text = {
"ONLY TIME WILL TELL." } },
},

/*  NO SCANNING, DID KILL LAIKA */
{
{   .lines = 2,
    .text = {
"THE CANINE SCOURGE",
"HAS BEEN ELIMINATED!", } },
{   .lines = 3,
    .text = {
"YOU HAVE KILLED LAIKA",
"AND ESCAPED BACK TO",
"PLANET EARTH." } },
{   .lines = 2,
    .text = {
"WILL HUMANITY EVER KNOW",
"WHAT THE DOGS WANTED?" } },
{   .lines = 1,
    .text = {
"WE MAY NEVER FIND OUT." } },
},

/*          THE GOOD ENDING           */
/*  HAVE SCANNING, DID NOT KILL LAIKA */
{
{   .lines = 5,
    .text = {
"YOU HAVE BROUGHT THE DOG",
"TRANSLATION TECHNOLOGY",
"TO EARTH, AND ALLOWED",
"THE DOGS TO REMAIN IN",
"SPACE." } },
{   .lines = 6,
    .text = {
"WITH THE HELP OF LAIKA AND",
"THE OTHER SPACE DOGS, THE",
"WORLD WILL ENTER A NEW AGE",
"OF SCIENTIFIC PROGRESS AND",
"EQUALITY BETWEEN ALL THE",
"CREATURES OF EARTH."} },
{   .lines = 4,
    .text = {
"WITH THEIR CANINE INGENUITY",
"HUMANITY CAN FINALLY OVER-",
"COME OUR RELIANCE UPON",
"EXPLOITATION AND HIERARCHY." } },
{   .lines = 5,
    .text = {
"HUMANS AND ANIMALS WILL BE",
"FINALLY ALLOWED TO LIVE IN",
"PEACE AND SECURITY.",
"",
"YOU HAVE SAVED US, COMRADE" } },
},

/*  GOT SCANNING, KILLED LAIKA  */
{
{   .lines = 5,
    .text = {
"THE CANINE SCOURGE HAS",
"BEEN ELIMINATED! YOU HAVE",
"KILLED THE COMMUNIST DOG",
"LAIKA, AND ESCAPED BACK",
"TO PLANET EARTH." } },
{   .lines = 4,
    .text = {
"THE DOGS CLAIMED THEY",
"WOULD SAVE HUMANITY FROM",
"ITSELF, BUT THAT WAS",
"JUST ANOTHER COMMIE LIE!" } },
{   .lines = 6,
    .text = {
"HIERARCHY IS HUMAN NATURE,",
"NOTHING A DOG MIGHT SAY",
"WILL EVER CHANGE THAT!",
"EXPLOITATION AND CRUELTY",
"ARE A NECESSARY EVIL OF",
"ANY CIVILIZATION." } },
{   .lines = 5,
    .text = {
"EVEN IF IT WAS POSSIBLE TO",
"MAKE IT BETTER...",
"",
"YOU HAVE MADE SURE THAT",
"THEY CANNOT TRY." } },
} };

static const struct intro_slide last_slides[4] = {
{   .lines = 1,
    .text = { "THE END" } },
{   .lines = 4,
    .text = {
"WOW! THANK YOU FOR PLAYING",
"THIS WEIRD GAME ABOUT",
"COMMUNIST SPACE DOGS.",
"I HOPE YOU ENJOYED IT!" } },
{   .lines = 6,
    .text = {
"DEVELOPED BY",
"JOSHUA GILES",
"TESTING AND ASSISTANCE FROM",
"KDRNIC",
"MUSIC BY ERIC MATYAS",
"WWW.SOUNDIMAGE.ORG" } },
{   .lines = 5,
    .text = {
"+ SUPER SPECIAL THANKS TO +",
"CAIN, STACEY, AND MY SISTER",
"KAITLYN, WITHOUT WHOSE",
"SUPPORT THIS GAME WOULD",
"NEVER HAVE BEEN POSSIBLE." } } };

void draw_outro(struct bork_play_data* d, float t)
{
    float ar = d->core->aspect_ratio;
    struct pg_shader* shader = &d->core->shader_2d;
    if(!pg_shader_is_active(shader)) pg_shader_begin(shader, NULL);
    pg_shader_2d_resolution(shader, (vec2){ ar, 1.0f });
    pg_shader_2d_set_light(&d->core->shader_2d, (vec2){}, (vec3){}, (vec3){ 1, 1, 1 });
    pg_model_begin(&d->core->quad_2d, &d->core->shader_2d);
    pg_shader_2d_transform(&d->core->shader_2d, (vec2){}, (vec2){ ar, 1 }, 0);
    pg_shader_2d_texture(&d->core->shader_2d, &d->core->radial_vignette);
    if(d->menu.intro.slide == 0 && d->menu.intro.ticks < PLAY_SECONDS(6)) {
        float alpha = (float)d->menu.intro.ticks / PLAY_SECONDS(6);
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){ 0, 0, 0, alpha },
                               (vec4){ 0, 0, 0, alpha });
    } else {
        pg_shader_2d_color_mod(&d->core->shader_2d, (vec4){}, (vec4){ 0, 0, 0, 1 });
    }
    pg_model_draw(&d->core->quad_2d, NULL);
    shader = &d->core->shader_text;
    pg_shader_begin(shader, NULL);
    pg_shader_text_resolution(shader, (vec2){ ar, 1 });
    pg_shader_text_transform(shader, (vec2){ 1, 1 }, (vec2){ 0, 0 });
    const struct intro_slide* slide = NULL;
    if(d->menu.intro.slide >= 8) return;
    if(d->menu.intro.slide >= 4) slide = &last_slides[d->menu.intro.slide - 4];
    else if(d->menu.intro.slide < 4) {
        int s = get_upgrade_level(d, BORK_UPGRADE_SCANNING) << 1;
        s |= d->killed_laika;
        slide = &slides[s][d->menu.intro.slide];
    }
    if(!slide) return;
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
    }
    pg_shader_text_write(shader, &text);
}


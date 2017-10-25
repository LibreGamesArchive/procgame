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
#include "state_play.h"
#include "game_states.h"

static const struct bork_upgrade_detail BORK_UPGRADE_DETAIL[] = {
    [BORK_UPGRADE_JETPACK] = { .name = "JETPACK",
        .description = {
            "SPACE-AGE MANEUVERING SYSTEM",
            "FOR MICRO-GRAVITY ENVIRONMENT.",
            "UPDATED MODEL CAPABLE OF",
            "OVERCOMING STANDARD GRAVITY." } },
    [BORK_UPGRADE_DOORHACK] = { .name = "DOOR HACK",
        .description = {
            "RE-USABLE DOOR OVERRIDE",
            "PACKAGE. UPDATED VERSION",
            "ALSO ALLOWS LOCKING DOORS",
            "AGAINST STATION ROBOTS." },
        .active = { 1, 1 },
        .keep_first = 1 },
    [BORK_UPGRADE_BOTHACK] = { .name = "BOT HACK",
        .description = {
            "SECURITY ROBOT OVERRIDE",
            "PACKAGE DISABLES ANY STATION",
            "MACHINERY. UPDATE ALLOWS",
            "TRIGGERING SELF-DESTRUCT",
            "FEATURE." },
        .active = { 1, 1 } },
    [BORK_UPGRADE_DECOY] = { .name = "DECOY",
        .description = {
            "SECURITY BEACON DIRECTING",
            "ALL STATION ROBOTS TO FOCUS",
            "ONE LOCATION. UPDATE ALLOWS",
            "PAINTING A TARGET AT RANGE." },
        .active = { 1, 1 } },
    [BORK_UPGRADE_HEALING] = { .name = "MEDICAL NANO-BOTS",
        .description = {
            "INTEGRATED MEDICAL NANO-BOTS",
            "TO PROVIDE ON-DEMAND HEALING",
            "SERVICE. UPDATE PROVIDES",
            "CONSTANT PASSIVE TISSUE REGEN",
            "REGENERATION." },
        .active = { 1, 0 } },
    [BORK_UPGRADE_HEATSHIELD] = { .name = "HEAT SHIELD",
        .description = {
            "FIRE RETARDANT EXO-SUIT",
            "PREVENTS USER FROM CATCHING",
            "FIRE. UPDATE PROVIDES TOTAL",
            "PROTECTION FROM HEAT DAMAGE" } },
    [BORK_UPGRADE_DEFENSE] = { .name = "DEFENSE FIELD",
        .description = {
            "PROJECTED DEFENSE FIELD CAN",
            "DAMAGE ALL NEARBY ATTACKERS.",
            "UPDATE PROVIDES CONSTANT",
            "PASSIVE PROTECTION FIELD." },
        .active = { 1, 1 },
        .keep_first = 0 },
    [BORK_UPGRADE_TELEPORTER] = { .name = "TRANSLOCATOR",
        .description = {
            "TRANSLOCATION BEACON ALLOWS",
            "USER TO TELEPORT INSTANTLY",
            "TO A PRE-SELECTED LOCATION.",
            "UPDATE TO ELIMINATE DISTANCE",
            "LIMITATION." },
        .active = { 1, 1 } },
    [BORK_UPGRADE_STRENGTH] = { .name = "STRENGTH",
        .description = {
            "MUSCLE ENHANCING NANO-BOTS",
            "GREATLY INCREASE MELEE DAMAGE.",
            "UPDATE FURTHER INCREASES",
            "DAMAGE AND KNOCKBACK." } },
    [BORK_UPGRADE_SCANNING] = { .name = "SCANNING",
        .description = {
            "OCULAR IMPLANT PERFORMS ACTIVE",
            "SCANNING OF VISIBLE OBJECTS AND",
            "PASSIVE TEXT/AUDIO TRANSLATION",
            "FROM RUSSIAN TO ENGLISH." },
        .active = { 1, 1 } },
};

const struct bork_upgrade_detail* bork_upgrade_detail(enum bork_upgrade u)
{
    return &BORK_UPGRADE_DETAIL[u];
}

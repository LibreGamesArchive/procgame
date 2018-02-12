enum bork_schematic {
    BORK_SCHEMATIC_BULLETS,
    BORK_SCHEMATIC_BULLETS_AP,
    BORK_SCHEMATIC_BULLETS_INC,
    BORK_SCHEMATIC_SHELLS,
    BORK_SCHEMATIC_SHELLS_AP,
    BORK_SCHEMATIC_SHELLS_INC,
    BORK_SCHEMATIC_FIRSTAID,
    BORK_SCHEMATIC_GRENADE_FRAG,
    BORK_SCHEMATIC_GRENADE_EMP,
    BORK_SCHEMATIC_GRENADE_INC,
    BORK_SCHEMATIC_BEAMSWORD,
    BORK_SCHEMATIC_PLAZGUN,
    BORK_SCHEMATIC_PLAZMA,
    BORK_SCHEMATIC_PLAZMA_SUPER,
    BORK_SCHEMATIC_PLAZMA_ICE,
    BORK_NUM_SCHEMATICS,
    BORK_SCHEMATIC_DOOR_OVERRIDE,
    BORK_SCHEMATIC_FIRE_EXT,
    BORK_SCHEMATIC_PROXY_FRAG,
    BORK_SCHEMATIC_PROXY_EMP,
    BORK_SCHEMATIC_BEARTRAP,
};

enum bork_resource {
    BORK_RESOURCE_FOODSTUFF,
    BORK_RESOURCE_CHEMICAL,
    BORK_RESOURCE_ELECTRICAL,
    BORK_RESOURCE_SCRAP,
};

static const struct bork_schematic_detail {
    uint8_t resources[4];
    enum bork_entity_type product;
} BORK_SCHEMATIC_DETAIL[] = {
    [BORK_SCHEMATIC_FIRSTAID] = { .product = BORK_ITEM_FIRSTAID,
        .resources = { 10, 1, 0, 10 } },
    [BORK_SCHEMATIC_GRENADE_EMP] = { .product = BORK_ITEM_GRENADE_EMP,
        .resources = { 0, 1, 10, 10 } },
    [BORK_SCHEMATIC_GRENADE_FRAG] = { .product = BORK_ITEM_GRENADE_FRAG,
        .resources = { 0, 1, 0, 10 } },
    [BORK_SCHEMATIC_GRENADE_INC] = { .product = BORK_ITEM_GRENADE_INC,
        .resources = { 0, 5, 0, 10 } },
    [BORK_SCHEMATIC_BEAMSWORD] = { .product = BORK_ITEM_BEAMSWORD,
        .resources = { 0, 15, 50, 15 } },
    [BORK_SCHEMATIC_PLAZGUN] = { .product = BORK_ITEM_PLAZGUN,
        .resources = { 0, 20, 50, 15 } },
    [BORK_SCHEMATIC_PLAZMA] = { .product = BORK_ITEM_PLAZMA,
        .resources = { 0, 10, 10, 5 } },
    [BORK_SCHEMATIC_PLAZMA_SUPER] = { .product = BORK_ITEM_PLAZMA_SUPER,
        .resources = { 0, 10, 10, 5 } },
    [BORK_SCHEMATIC_PLAZMA_ICE] = { .product = BORK_ITEM_PLAZMA_ICE,
        .resources = { 0, 15, 10, 5 } },
    [BORK_SCHEMATIC_BULLETS] = { .product = BORK_ITEM_BULLETS,
        .resources = { 0, 1, 0, 10 } },
    [BORK_SCHEMATIC_BULLETS_AP] = { .product = BORK_ITEM_BULLETS_AP,
        .resources = { 0, 1, 0, 15 } },
    [BORK_SCHEMATIC_BULLETS_INC] = { .product = BORK_ITEM_BULLETS_INC,
        .resources = { 0, 5, 0, 15 } },
    [BORK_SCHEMATIC_SHELLS] = { .product = BORK_ITEM_SHELLS,
        .resources = { 0, 1, 0, 10 } },
    [BORK_SCHEMATIC_SHELLS_AP] = { .product = BORK_ITEM_SHELLS_AP,
        .resources = { 0, 1, 0, 15 } },
    [BORK_SCHEMATIC_SHELLS_INC] = { .product = BORK_ITEM_SHELLS_INC,
        .resources = { 0, 5, 0, 15 } },
};

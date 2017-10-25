struct bork_play_data;

enum bork_upgrade {
    BORK_UPGRADE_JETPACK,
    BORK_UPGRADE_DOORHACK,
    BORK_UPGRADE_BOTHACK,
    BORK_UPGRADE_DECOY,
    BORK_UPGRADE_HEALING,
    BORK_UPGRADE_DEFENSE,
    BORK_UPGRADE_TELEPORTER,
    BORK_UPGRADE_HEATSHIELD,
    BORK_UPGRADE_STRENGTH,
    BORK_UPGRADE_SCANNING,
    BORK_NUM_UPGRADES
};

struct bork_upgrade_detail {
    char name[32];
    char description[8][32];
    int active[2];
    int frame;
    int keep_first;
};
const struct bork_upgrade_detail* bork_upgrade_detail(enum bork_upgrade u);

void bork_use_upgrade(struct bork_play_data* d, enum bork_upgrade u, int l);
void select_next_upgrade(struct bork_play_data* d);



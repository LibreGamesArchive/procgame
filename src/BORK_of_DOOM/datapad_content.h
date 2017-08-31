static const int NUM_DATAPADS = 2;

static const struct bork_datapad {
    char title[64];
    char text[32][32];
    int lines;
} BORK_DATAPADS[] = {
    { .title = "MYSTERIOUS DATAPAD", .lines = 4,
      .text = { "THE STATION AI HAS TAKEN",
                "OVER! IF IT DID NOT CHANGE",
                "THE CODE, YOU SHOULD BE",
                "ABLE TO USE '0451' TO ESCAPE!" } },
    { .title = "NOT COMING HOME", .lines = 5,
      .text = { "IF YOU ARE READING THIS I",
                "MUST BE DEAD... I LOCKED",
                "MYSELF IN HERE AND CHANGED",
                "THE DOOR CODE TO 4124. TELL",
                "MY FAMILY I LOVE THEM." } },
};

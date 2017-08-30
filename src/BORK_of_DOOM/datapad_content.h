static const int NUM_DATAPADS = 2;

static const struct bork_datapad {
    char title[64];
    char text[15][64];
    int lines;
} BORK_DATAPADS[] = {
    { .title = "MYSTERIOUS DATAPAD", .lines = 4,
      .text = { "THE STATION AI HAS TAKEN",
                "OVER! IF IT DID NOT CHANGE",
                "THE CODE, YOU SHOULD BE",
                "ABLE TO USE '0451' TO ESCAPE!" } },
    { .title = "COMMUNISM WILL WIN", .lines = 1,
      .text = { "A SPECTER LOOMS OVER THE PLANET EARTH" } },
};

#define NUM_DATAPADS    2

static const struct bork_datapad {
    char title[64];
    char text[32][32];
    int lines;
} BORK_DATAPADS[] = {
    { .title = "MYSTERIOUS DATAPAD", .lines = 8,
      .text = { "THE DOGS HAVE TAKEN OVER",
                "THE STATION! THEY MUST BE",
                "STOPPED! I HAVE OVERRIDDEN",
                "YOUR DOOR LOCK SO THEY",
                "CANNOT GET TO YOU. THERE",
                "SHOULD BE SOME WEAPONS STASHED",
                "IN THE ROOM ACROSS FROM YOURS.",
                "THE CODE TO GET IN IS '0451'." } },
    { .title = "NOT COMING HOME", .lines = 5,
      .text = { "IF YOU ARE READING THIS I",
                "MUST BE DEAD... I LOCKED",
                "MYSELF IN HERE AND CHANGED",
                "THE DOOR CODE TO 4124. TELL",
                "MY FAMILY I LOVE THEM." } },
};

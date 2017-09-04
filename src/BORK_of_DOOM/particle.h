#define BORK_PARTICLE_SPRITE        (1 << 0)
#define BORK_PARTICLE_LIGHT         (1 << 1)
#define BORK_PARTICLE_GRAVITY       (1 << 2)
#define BORK_PARTICLE_BOUYANT       (1 << 3)
#define BORK_PARTICLE_LOOP_ANIM     (1 << 4)
#define BORK_PARTICLE_LIGHT_DECAY   (1 << 5)
#define BORK_PARTICLE_DECELERATE    (1 << 6)

struct bork_particle {
    vec3 pos;
    vec3 vel;
    vec4 light;
    uint8_t color_mod[3];
    uint8_t start_frame, end_frame, current_frame;
    uint8_t flags;
    uint8_t frame_ticks;
    uint16_t lifetime;
    uint16_t ticks_left;
};

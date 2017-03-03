#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "../linmath.h"
#include "wave.h"
#include "texture.h"
#include "shader.h"
#include "postproc.h"
#include "vertex.h"
#include "model.h"
#include "shape.h"
#include "gbuffer.h"

int pg_init(int w, int h, int fullscreen, const char* window_title);
void pg_deinit(void);
void pg_screen_swap(void);
void pg_screen_dst(void);

/*  Returns the time in seconds since the last call to this function, or 0
    the first time it is called. If dump is non-zero, the value will not be
    used toward framerate calculations and will not reset the delta time for
    the next call   */
float pg_delta_time(int dump);
/*  Returns a running average framerate based on calls to pg_delta_time()   */
float pg_framerate(void);

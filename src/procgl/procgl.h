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

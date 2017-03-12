CC := gcc
CC_WINDOWS := i686-w64-mingw32-gcc
CFLAGS_DBG := -Wall -g -O0
CFLAGS_RELEASE := -Wall -O2
INCLUDES := -Isrc
# On Linux, only static link with my custom libcurl
LIBS_LINUX := -l:src/libs/linux/libcurl/libcurl.a \
 -lcrypto -lssl -lSDL2 -lGL -lGLEW -lm
# On Windows, static link with custom libcurl, GLEW, and SDL2
LIBS_WINDOWS := -lmingw32 -l:src/libs/win32/GL/libglew32.a \
 -l:src/libs/win32/SDL2/libSDL2.a \
 -l:src/libs/win32/SDL2/libSDL2main.a \
 -l:src/libs/win32/libcurl/libcurl.a \
 -lmingw32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion \
 -lopengl32 -lws2_32 -lcrypt32

TARGET := procgame

GAME := game_state.o game_example.o
PROCGL := procgl_base.o viewer.o postproc.o shader.o gbuffer.o \
 model.o model_prims.o shape.o shape_prims.o texture.o wave.o \
 shader_2d.o shader_3d.o shader_text.o
PROCGL_LIBS := lodepng.o noise1234.o

debug_linux: INCLUDES += -Isrc/libs/linux
debug_linux: LIBS := $(LIBS_LINUX)
debug_linux: CFLAGS := $(CFLAGS_DEBUG)
debug_linux: procgame

release_linux: INCLUDES += -Isrc/libs/linux
release_linux: LIBS := $(LIBS_LINUX)
release_linux: CFLAGS := $(CFLAGS_RELEASE) -DPROCGL_STATIC_SHADERS
release_linux: clean dump_shaders procgame

release_win32: INCLUDES += -Isrc/libs/win32
release_win32: CC := $(CC_WINDOWS)
release_win32: LIBS := $(LIBS_WINDOWS)
release_win32: CFLAGS := $(CFLAGS_RELEASE) -DPROCGL_STATIC_SHADERS -DGLEW_STATIC
release_win32: TARGET := $(TARGET).exe
release_win32: clean dump_shaders procgame

procgame: main.o
	$(CC) -o $(TARGET) main.o $(GAME) $(PROCGL) $(PROCGL_LIBS) $(LIBS)

main.o: src/main.c $(GAME) $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -c src/main.c $(INCLUDES)

game_state.o: src/game/game_state.c src/game/game_state.h
	$(CC) $(CFLAGS) -c src/game/game_state.c $(INCLUDES)
game_example.o: src/game/game_example.c src/game/game_example.h \
 $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -c src/game/game_example.c $(INCLUDES)

procgl_base.o: src/procgl/procgl_base.c src/procgl/procgl_base.h
	$(CC) $(CFLAGS) -c src/procgl/procgl_base.c $(INCLUDES)
viewer.o: src/procgl/viewer.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h
	$(CC) $(CFLAGS) -c src/procgl/viewer.c $(INCLUDES)
postproc.o: src/procgl/postproc.c src/procgl/ext/linmath.h \
 src/procgl/postproc.h src/procgl/viewer.h src/procgl/shader.h
	$(CC) $(CFLAGS) -c src/procgl/postproc.c $(INCLUDES)
shader.o: src/procgl/shader.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h src/procgl/shader.h
	$(CC) $(CFLAGS) -c src/procgl/shader.c $(INCLUDES)
gbuffer.o: src/procgl/gbuffer.c src/procgl/ext/linmath.h \
 src/procgl/texture.h src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/gbuffer.h
	$(CC) $(CFLAGS) -c src/procgl/gbuffer.c $(INCLUDES)
model.o: src/procgl/model.c src/procgl/ext/linmath.h src/procgl/vertex.h \
 src/procgl/arr.h src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/texture.h src/procgl/model.h
	$(CC) $(CFLAGS) -c src/procgl/model.c $(INCLUDES)
model_prims.o: src/procgl/model_prims.c src/procgl/ext/linmath.h \
 src/procgl/vertex.h src/procgl/arr.h src/procgl/texture.h \
 src/procgl/viewer.h src/procgl/shader.h src/procgl/model.h
	$(CC) $(CFLAGS) -c src/procgl/model_prims.c $(INCLUDES)
shape.o: src/procgl/shape.c src/procgl/ext/linmath.h src/procgl/vertex.h \
 src/procgl/arr.h src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/shape.h
	$(CC) $(CFLAGS) -c src/procgl/shape.c $(INCLUDES)
shape_prims.o: src/procgl/shape_prims.c src/procgl/ext/linmath.h \
 src/procgl/vertex.h src/procgl/arr.h src/procgl/viewer.h \
 src/procgl/shader.h src/procgl/texture.h src/procgl/shape.h
	$(CC) $(CFLAGS) -c src/procgl/shape_prims.c $(INCLUDES)
texture.o: src/procgl/texture.c src/procgl/ext/noise1234.h \
 src/procgl/ext/lodepng.h src/procgl/ext/linmath.h src/procgl/texture.h
	$(CC) $(CFLAGS) -c src/procgl/texture.c $(INCLUDES)
wave.o: src/procgl/wave.c src/procgl/wave.h
	$(CC) $(CFLAGS) -c src/procgl/wave.c $(INCLUDES)
shader_2d.o: src/procgl/shaders/shader_2d.c src/procgl/ext/linmath.h \
 src/procgl/vertex.h src/procgl/texture.h \
 src/procgl/viewer.h src/procgl/shader.h 
	$(CC) $(CFLAGS) -c src/procgl/shaders/shader_2d.c $(INCLUDES)
shader_3d.o: src/procgl/shaders/shader_3d.c src/procgl/ext/linmath.h \
 src/procgl/vertex.h src/procgl/texture.h \
 src/procgl/viewer.h src/procgl/shader.h
	$(CC) $(CFLAGS) -c src/procgl/shaders/shader_3d.c $(INCLUDES)
shader_text.o: src/procgl/shaders/shader_text.c src/procgl/ext/linmath.h \
 src/procgl/texture.h src/procgl/viewer.h src/procgl/shader.h
	$(CC) $(CFLAGS) -c src/procgl/shaders/shader_text.c $(INCLUDES)

lodepng.o: src/procgl/ext/lodepng.c src/procgl/ext/lodepng.h
	$(CC) $(CFLAGS) -c src/procgl/ext/lodepng.c $(INCLUDES)
noise1234.o: src/procgl/ext/noise1234.c src/procgl/ext/noise1234.h
	$(CC) $(CFLAGS) -c src/procgl/ext/noise1234.c $(INCLUDES)

dump_shaders: src/procgl/shaders/*.glsl
	cd src/procgl/shaders && \
    xxd -i text_vert.glsl >> text.glsl.h && \
    xxd -i text_frag.glsl >> text.glsl.h && \
    xxd -i 2d_vert.glsl >> 2d.glsl.h && \
    xxd -i 2d_frag.glsl >> 2d.glsl.h && \
    xxd -i 3d_vert.glsl >> 3d.glsl.h && \
    xxd -i 3d_frag.glsl >> 3d.glsl.h && \
    xxd -i deferred_vert.glsl >> deferred.glsl.h && \
    xxd -i deferred_frag.glsl >> deferred.glsl.h && \
    xxd -i screen_vert.glsl >> deferred.glsl.h && \
    xxd -i screen_frag.glsl >> deferred.glsl.h

clean:
	rm -f *.o
	rm -f src/procgl/shaders/*.glsl.h


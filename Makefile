CC := gcc
CC_WINDOWS := i686-w64-mingw32-gcc
CFLAGS_DEBUG := -Wall -g -O0
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

GAME := obj/game_state.o obj/bork.o obj/state_menu.o obj/state_play.o \
    obj/map_area.o obj/physics.o
PROCGL := obj/procgl_base.o \
 obj/viewer.o obj/postproc.o obj/shader.o obj/gbuffer.o \
 obj/model.o obj/model_prims.o obj/marching_cubes.o \
 obj/shader_2d.o obj/shader_3d.o obj/shader_cubetex.o obj/shader_text.o \
 obj/wave.o obj/heightmap.o obj/texture.o obj/audio.o \
 obj/sdf.o obj/sdf_functions.o
PROCGL_LIBS := obj/lodepng.o obj/noise1234.o obj/wavfile.o obj/easing.o

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

procgame: obj/main.o
	$(CC) -o $(TARGET) obj/main.o $(GAME) $(PROCGL) $(PROCGL_LIBS) $(LIBS)

obj/main.o: src/main.c $(GAME) $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -o obj/main.o -c src/main.c $(INCLUDES)

obj/game_state.o: src/game/game_state.c src/game/game_state.h
	$(CC) $(CFLAGS) -o obj/game_state.o -c src/game/game_state.c $(INCLUDES)
obj/bork.o: src/BORK_of_DOOM/bork.c src/BORK_of_DOOM/bork.h \
 $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -o obj/bork.o -c src/BORK_of_DOOM/bork.c $(INCLUDES)
obj/state_menu.o: src/BORK_of_DOOM/state_menu.c src/BORK_of_DOOM/bork.h \
 $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -o obj/state_menu.o -c src/BORK_of_DOOM/state_menu.c $(INCLUDES)
obj/state_play.o: src/BORK_of_DOOM/state_play.c src/BORK_of_DOOM/bork.h \
 $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -o obj/state_play.o -c src/BORK_of_DOOM/state_play.c $(INCLUDES)
obj/map_area.o: src/BORK_of_DOOM/map_area.c src/BORK_of_DOOM/map_area.h \
 $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -o obj/map_area.o -c src/BORK_of_DOOM/map_area.c $(INCLUDES)
obj/physics.o: src/BORK_of_DOOM/physics.c src/BORK_of_DOOM/map_area.h \
 $(PROCGL) $(PROCGL_LIBS)
	$(CC) $(CFLAGS) -o obj/physics.o -c src/BORK_of_DOOM/physics.c $(INCLUDES)

obj/procgl_base.o: src/procgl/procgl_base.c src/procgl/procgl_base.h
	$(CC) $(CFLAGS) -o obj/procgl_base.o -c src/procgl/procgl_base.c $(INCLUDES)
obj/viewer.o: src/procgl/viewer.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h
	$(CC) $(CFLAGS) -o obj/viewer.o -c src/procgl/viewer.c $(INCLUDES)
obj/postproc.o: src/procgl/postproc.c src/procgl/ext/linmath.h \
 src/procgl/postproc.h src/procgl/viewer.h src/procgl/shader.h
	$(CC) $(CFLAGS) -o obj/postproc.o -c src/procgl/postproc.c $(INCLUDES)
obj/shader.o: src/procgl/shader.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h src/procgl/shader.h
	$(CC) $(CFLAGS) -o obj/shader.o -c src/procgl/shader.c $(INCLUDES)
obj/gbuffer.o: src/procgl/gbuffer.c src/procgl/ext/linmath.h \
 src/procgl/texture.h src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/gbuffer.h
	$(CC) $(CFLAGS) -o obj/gbuffer.o -c src/procgl/gbuffer.c $(INCLUDES)
obj/model.o: src/procgl/model.c src/procgl/ext/linmath.h  \
 src/procgl/arr.h src/procgl/viewer.h src/procgl/shader.h src/procgl/model.h \
 src/procgl/wave.h src/procgl/heightmap.h src/procgl/texture.h
	$(CC) $(CFLAGS) -o obj/model.o -c src/procgl/model.c $(INCLUDES)
obj/model_prims.o: src/procgl/model_prims.c src/procgl/ext/linmath.h \
  src/procgl/arr.h src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/model.h src/procgl/wave.h src/procgl/heightmap.h src/procgl/texture.h
	$(CC) $(CFLAGS) -o obj/model_prims.o -c src/procgl/model_prims.c $(INCLUDES)
obj/heightmap.o: src/procgl/heightmap.c src/procgl/heightmap.h \
 src/procgl/wave.h
	$(CC) $(CFLAGS) -o obj/heightmap.o -c src/procgl/heightmap.c $(INCLUDES)
obj/texture.o: src/procgl/texture.c src/procgl/ext/noise1234.h \
 src/procgl/ext/lodepng.h src/procgl/ext/linmath.h src/procgl/texture.h \
 src/procgl/heightmap.h src/procgl/wave.h
	$(CC) $(CFLAGS) -o obj/texture.o -c src/procgl/texture.c $(INCLUDES)
obj/shader_2d.o: src/procgl/shaders/shader_2d.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/wave.h src/procgl/heightmap.h src/procgl/texture.h
	$(CC) $(CFLAGS) -o obj/shader_2d.o -c src/procgl/shaders/shader_2d.c $(INCLUDES)
obj/shader_3d.o: src/procgl/shaders/shader_3d.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/wave.h src/procgl/heightmap.h src/procgl/texture.h
	$(CC) $(CFLAGS) -o obj/shader_3d.o -c src/procgl/shaders/shader_3d.c $(INCLUDES)
obj/shader_cubetex.o: src/procgl/shaders/shader_cubetex.c \
 src/procgl/ext/linmath.h src/procgl/viewer.h src/procgl/shader.h \
 src/procgl/wave.h src/procgl/heightmap.h src/procgl/texture.h
	$(CC) $(CFLAGS) -o obj/shader_cubetex.o -c src/procgl/shaders/shader_cubetex.c $(INCLUDES)
obj/shader_text.o: src/procgl/shaders/shader_text.c src/procgl/ext/linmath.h \
 src/procgl/viewer.h src/procgl/shader.h src/procgl/procgl_base.h \
 src/procgl/wave.h src/procgl/heightmap.h src/procgl/texture.h
	$(CC) $(CFLAGS) -o obj/shader_text.o -c src/procgl/shaders/shader_text.c $(INCLUDES)
obj/wave.o: src/procgl/wave.c src/procgl/wave.h
	$(CC) $(CFLAGS) -o obj/wave.o -c src/procgl/wave.c $(INCLUDES)
obj/audio.o: src/procgl/audio.c src/procgl/audio.h \
 src/procgl/wave.h src/procgl/arr.h src/procgl/ext/wavfile.h
	$(CC) $(CFLAGS) -o obj/audio.o -c src/procgl/audio.c $(INCLUDES)
obj/sdf.o: src/procgl/sdf.c src/procgl/sdf.h \
 src/procgl/wave.h src/procgl/arr.h
	gperf src/procgl/sdf_keywords.gperf \
        -H pg_sdf_keyword_hash -N pg_sdf_keyword_read -t --omit-struct-type \
        > src/procgl/sdf_keywords.gperf.c &
	$(CC) $(CFLAGS) -o obj/sdf.o -c src/procgl/sdf.c $(INCLUDES)
obj/sdf_functions.o: src/procgl/sdf_functions.c src/procgl/sdf.h \
 src/procgl/wave.h src/procgl/arr.h
	$(CC) $(CFLAGS) -o obj/sdf_functions.o -c src/procgl/sdf_functions.c $(INCLUDES)
obj/marching_cubes.o: src/procgl/marching_cubes.c src/procgl/sdf.h \
 src/procgl/arr.h src/procgl/ext/linmath.h src/procgl/model.h
	$(CC) $(CFLAGS) -o obj/marching_cubes.o -c src/procgl/marching_cubes.c $(INCLUDES)

obj/lodepng.o: src/procgl/ext/lodepng.c src/procgl/ext/lodepng.h
	$(CC) $(CFLAGS) -o obj/lodepng.o -c src/procgl/ext/lodepng.c $(INCLUDES)
obj/noise1234.o: src/procgl/ext/noise1234.c src/procgl/ext/noise1234.h
	$(CC) $(CFLAGS) -o obj/noise1234.o -c src/procgl/ext/noise1234.c $(INCLUDES)
obj/wavfile.o: src/procgl/ext/wavfile.c src/procgl/ext/wavfile.h
	$(CC) $(CFLAGS) -o obj/wavfile.o -c src/procgl/ext/wavfile.c $(INCLUDES)
obj/easing.o: src/procgl/ext/AHEasing/easing.c src/procgl/ext/AHEasing/easing.h
	$(CC) $(CFLAGS) -o obj/easing.o -c src/procgl/ext/AHEasing/easing.c $(INCLUDES)

dump_shaders: src/procgl/shaders/*.glsl
	cd src/procgl/shaders && \
    xxd -i text_vert.glsl >> text.glsl.h && \
    xxd -i text_frag.glsl >> text.glsl.h && \
    xxd -i 2d_vert.glsl >> 2d.glsl.h && \
    xxd -i 2d_frag.glsl >> 2d.glsl.h && \
    xxd -i 3d_vert.glsl >> 3d.glsl.h && \
    xxd -i 3d_frag.glsl >> 3d.glsl.h && \
    xxd -i cubetex_vert.glsl >> cubetex.glsl.h && \
    xxd -i cubetex_frag.glsl >> cubetex.glsl.h && \
    xxd -i deferred_vert.glsl >> deferred.glsl.h && \
    xxd -i deferred_frag.glsl >> deferred.glsl.h && \
    xxd -i screen_vert.glsl >> deferred.glsl.h && \
    xxd -i screen_frag.glsl >> deferred.glsl.h

clean:
	rm -f obj/*.o
	rm -f src/procgl/shaders/*.glsl.h


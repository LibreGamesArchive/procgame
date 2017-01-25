cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_MODULE_PATH "./cmake")
project(procgl)

file(GLOB SOURCES src/*.c src/procgl/*.c)
add_executable(procgl ${SOURCES})

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries(procgl ${GLEW_LIBRARIES})
endif()

find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIR})
    target_link_libraries(procgl ${SDL2_LIBRARY})
endif()

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIR})
    target_link_libraries(procgl ${OPENGL_LIBRARY})
endif()

target_link_libraries(procgl m)


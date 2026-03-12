cmake_minimum_required(VERSION 3.16)

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)

project(zpac C)

add_executable(zpac
    src/main.c
    src/maze_logic.c
    src/input.c
    src/game_loop.c
    src/dots.c
    src/ghost.c
    src/sound.c
    src/fruit.c
    src/level256.c
    src/cutscene.c
)

target_include_directories(zpac PRIVATE src)

find_package(ZVB_SDK REQUIRED)
target_link_libraries(zpac PUBLIC zvb_gfx zvb_sound)

zos_add_outputs(zpac)

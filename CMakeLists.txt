cmake_minimum_required(VERSION 3.16)

set(ZOS_TOOLCHAIN sdcc)
include($ENV{ZOS_PATH}/cmake/zos_init.cmake)

zos_use(zvb)

project(zpac_test C)

add_executable(zpac_test
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

target_include_directories(zpac_test PRIVATE src)

find_package(ZVB_SDK REQUIRED)
target_link_libraries(zpac_test PUBLIC zvb_gfx zvb_sound)

zos_add_outputs(zpac_test)

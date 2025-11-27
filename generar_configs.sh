#!/bin/bash

echo "--- Generando archivos de configuración para LVGL ---"

# 1. Crear lib/lv_conf.h
cat > lib/lv_conf.h <<EOF
#ifndef LV_CONF_H
#define LV_CONF_H
#include <stdint.h>

#define LV_COLOR_DEPTH 32
#define LV_COLOR_16_SWAP 0
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (128 * 1024U)
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_28 1

#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TABLE 1
#define LV_USE_CANVAS 1
#endif
EOF
echo "[OK] lib/lv_conf.h creado."

# 2. Crear lib/lv_drv_conf.h
cat > lib/lv_drv_conf.h <<EOF
#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H
#include "lv_conf.h"

#ifndef USE_SDL
# define USE_SDL 1
#endif

#if USE_SDL
    #define SDL_HOR_RES     800
    #define SDL_VER_RES     480
    #define SDL_ZOOM        1
    #define SDL_DOUBLE_BUFFER 0
    #define SDL_INCLUDE_PATH <SDL2/SDL.h>
#endif

#ifndef USE_MOUSE
# define USE_MOUSE 1
#endif
#ifndef USE_MOUSEWHEEL
# define USE_MOUSEWHEEL 1
#endif
#ifndef USE_KEYBOARD
# define USE_KEYBOARD 1
#endif
#endif
EOF
echo "[OK] lib/lv_drv_conf.h creado."

# 3. Sobrescribir CMakeLists.txt
cat > CMakeLists.txt <<EOF
cmake_minimum_required(VERSION 3.10)
project(CNC_Gateway_HMI C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(
    src
    lib
    lib/lvgl
    lib/lv_drivers
    \${SDL2_INCLUDE_DIRS}
)

add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)
add_definitions(-DLV_DRV_NO_CONF)

set(SOURCES
    src/main.c
    src/mqtt/mqtt_service.c
)

file(GLOB_RECURSE LVGL_SOURCES "lib/lvgl/src/*.c")
file(GLOB_RECURSE LV_DRIVERS_SOURCES "lib/lv_drivers/*.c")

add_executable(cnc_app
    \${SOURCES}
    \${LVGL_SOURCES}
    \${LV_DRIVERS_SOURCES}
)

target_link_libraries(cnc_app
    paho-mqtt3a
    pthread
    \${SDL2_LIBRARIES}
    m
)
EOF
echo "[OK] CMakeLists.txt actualizado."
echo "--- ¡Listo! Ahora puedes compilar. ---"


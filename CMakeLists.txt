cmake_minimum_required(VERSION 3.10)
project(CNC_Gateway_HMI C)

set(CMAKE_C_STANDARD 11)

# Paquetes requeridos
find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)
# find_package(CURL REQUIRED) # Comenta esto si no vas a usar AWS por ahora

# Directorios donde buscar archivos .h (Header files)
include_directories(
    src
    src/files
    src/logger
    src/ui
    src/mqtt
    # src/aws # Comenta esto si no usas AWS
    lib
    lib/lvgl
    lib/lv_drivers
    ${SDL2_INCLUDE_DIRS}
    # ${CURL_INCLUDE_DIRS}
)

# Definiciones de configuración LVGL
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)

# --- ARCHIVOS FUENTE (SOURCES) ---

# 1. Archivos base del sistema (Los que escribiste a mano)
set(SOURCES
    src/main.c
    src/mqtt/mqtt_service.c
    src/files/file_manager.c
    src/logger/logger.c
    # src/aws/aws_service.c # Comenta esto
    # NO pongas archivos de UI aquí manualmente
)

# 2. Archivos de Librerías (Búsqueda Automática)
file(GLOB_RECURSE LVGL_SOURCES "lib/lvgl/src/*.c")
file(GLOB_RECURSE LV_DRIVERS_SOURCES "lib/lv_drivers/*.c")

# 3. Archivos de UI (Búsqueda Automática - LA SOLUCIÓN)
# Esto encontrará ui.c, ui_events.c y todos los screens/*.c automáticamente
file(GLOB_RECURSE UI_GENERATED "src/ui/*.c")

# --- CREAR EJECUTABLE ---
add_executable(cnc_app
    ${SOURCES}
    ${LVGL_SOURCES}
    ${LV_DRIVERS_SOURCES}
    ${UI_GENERATED}
)

# Enlace de librerías
target_link_libraries(cnc_app
    paho-mqtt3a
    pthread
    ${SDL2_LIBRARIES}
    # ${CURL_LIBRARIES}
    m
)

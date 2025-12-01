cmake_minimum_required(VERSION 3.10)
project(CNC_Gateway_HMI C)

set(CMAKE_C_STANDARD 11)

find_package(Threads REQUIRED)
find_package(SDL2 REQUIRED)

# Directorios donde buscar archivos .h
include_directories(
    src             # <--- Aquí está lv_conf.h
    src/ui
    src/files
    src/logger
    src/mqtt
    lib
    lib/lvgl
    lib/lv_drivers
    ${SDL2_INCLUDE_DIRS}
)

# Importante para que LVGL busque la configuración
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)

# Archivos Fuente de TU proyecto
# BUSCAR AUTOMÁTICAMENTE TODOS LOS ARCHIVOS DE LA UI
file(GLOB_RECURSE UI_SOURCES "src/ui/*.c")

set(SOURCES
    src/main.c
    src/mqtt/mqtt_service.c
    src/files/file_manager.c
    src/logger/logger.c
    # Ya no ponemos ui_manager.c aquí
)


# Archivos Fuente de la Librería (Búsqueda automática)
file(GLOB_RECURSE LVGL_SOURCES "lib/lvgl/src/*.c")
file(GLOB_RECURSE LV_DRIVERS_SOURCES "lib/lv_drivers/*.c")

add_executable(cnc_app
    ${SOURCES}
    ${UI_SOURCES}      # <--- Agregamos los archivos de SquareLine
    ${LVGL_SOURCES}
    ${LV_DRIVERS_SOURCES}
)

target_link_libraries(cnc_app paho-mqtt3a pthread ${SDL2_LIBRARIES} m)

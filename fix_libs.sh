#!/bin/bash

echo "--- DIAGNÓSTICO DE LIBRERÍAS ---"

DIR_LVGL="lib/lvgl"
FILE_LVGL="lib/lvgl/lvgl.h"
DIR_DRIVERS="lib/lv_drivers"

# 1. Verificar si existe la carpeta y el archivo clave
if [ -f "$FILE_LVGL" ]; then
    echo "[OK] LVGL parece estar instalado correctamente."
else
    echo "[ERROR] No se encontró lvgl.h. La carpeta parece vacía o dañada."
    echo "--- REPARANDO (Descargando de nuevo) ---"

    # Borrar carpetas corruptas/vacías
    rm -rf lib/lvgl lib/lv_drivers

    mkdir -p lib
    cd lib

    # Descargar de nuevo (mostrando progreso)
    echo "Descargando LVGL Core..."
    git clone -b release/v8.3 --depth 1 https://github.com/lvgl/lvgl.git

    echo "Descargando LVGL Drivers..."
    git clone -b release/v8.2 --depth 1 https://github.com/lvgl/lv_drivers.git

    cd ..
    echo "[LISTO] Descarga completada."
fi

echo "--- LIMPIEZA DE CACHE DE COMPILACIÓN ---"
# Esto es vital: Si CMake recuerda que falló antes, fallará de nuevo aunque ya existan los archivos
rm -rf build
mkdir build

echo "--- EJECUTANDO CMAKE ---"
# Configuramos el proyecto desde cero
cmake -B build

echo "--- ¡TODO LISTO! ---"
echo "Intenta compilar ahora con: cmake --build build"


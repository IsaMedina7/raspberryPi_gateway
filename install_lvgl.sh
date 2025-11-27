#!/bin/bash

echo "--- 1. Instalando Driver Gráfico (SDL2) ---"
# SDL2 permite que LVGL dibuje en la pantalla de la Raspberry
sudo apt-get update
sudo apt-get install -y libsdl2-dev

echo "--- 2. Descargando LVGL (Versión 8.3 LTS) ---"
cd lib

# Si ya existe, borramos para empezar limpio
rm -rf lvgl lv_drivers

# Clonamos el núcleo de LVGL
git clone -b release/v8.3 https://github.com/lvgl/lvgl.git

# Clonamos los drivers para Linux (Teclado, Mouse, Pantalla)
git clone -b release/v8.2 https://github.com/lvgl/lv_drivers.git

echo "--- 3. Preparando archivos de configuración ---"
# LVGL necesita que movamos sus archivos de config a la raíz de 'lib'
# (Los crearemos manualmente en el siguiente paso para asegurarnos que están bien)

echo "--- ¡INSTALACIÓN COMPLETA! ---"
echo "Ahora necesitamos crear los archivos de configuración (lv_conf.h)."


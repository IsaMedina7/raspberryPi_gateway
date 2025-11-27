#!/bin/bash
# Script de configuración inicial para Raspberry Pi 3 (Bookworm)
# Ejecutar con: chmod +x setup_project.sh && ./setup_project.sh

echo "--- 1. Creando directorios del proyecto ---"
mkdir -p ~/proyecto_cnc_hmi/src/mqtt
mkdir -p ~/proyecto_cnc_hmi/lib
mkdir -p ~/proyecto_cnc_hmi/build

echo "--- 2. Descargando y Compilando Eclipse Paho MQTT C ---"
cd ~/proyecto_cnc_hmi/lib

# Clonamos el repositorio oficial
if [ ! -d "paho.mqtt.c" ]; then
    git clone https://github.com/eclipse/paho.mqtt.c.git
fi

cd paho.mqtt.c
git checkout v1.3.13 # Usamos una versión estable probada

# Compilamos e instalamos en el sistema
# -DPAHO_WITH_SSL=TRUE es necesario para AWS en el futuro
cmake -Bbuild -H. -DPAHO_WITH_SSL=TRUE -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=FALSE
sudo cmake --build build/ --target install
sudo ldconfig # Actualiza los enlaces de librerías del sistema

echo "--- 3. Instalación completada ---"
echo "La librería Paho MQTT ha sido instalada correctamente."

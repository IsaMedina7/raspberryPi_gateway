#!/usr/bin/env python3
import requests
import os
import time
import re

# --- CONFIGURACIÓN ---
SERVER_URL = "http://18.223.169.118:80"
API_LISTA = f"{SERVER_URL}/api/ordenes"
STATUS_FILE = "/home/merry/proyecto_cnc_hmi/db_status.txt"
JSON_FILE = "/home/merry/proyecto_cnc_hmi/ordenes.json"

# Carpeta donde guardaremos los archivos en la Raspberry
CARPETA_DESCARGAS = "/home/merry/proyecto_cnc_hmi/gcode_files"

# ID de esta máquina (para filtrar solo lo que le corresponde a ella)
MI_ID_MAQUINA = "cnc1"

# --- PREPARACIÓN DEL SISTEMA ---
if not os.path.exists(CARPETA_DESCARGAS):
    os.makedirs(CARPETA_DESCARGAS)
    print(f"✅ Carpeta '{CARPETA_DESCARGAS}' creada.")

# -------------------------------
#   FUNCIONES AUXILIARES
# -------------------------------
def actualizar_estado_db(estado):
    """Escribe OK o ERROR en un archivo para que el programa en C lo lea."""
    try:
        with open(STATUS_FILE, "w") as f:
            f.write(estado)
    except Exception as e:
        print(f"[ERROR] No se pudo escribir {STATUS_FILE}: {e}")

def limpiar_nombre(nombre):
    """Reemplaza caracteres inválidos para nombres de archivo."""
    return re.sub(r'[^a-zA-Z0-9_.-]', '_', nombre)

def descargar_archivo(id_orden, ruta_guardado):
    """Descarga un archivo de la orden desde el servidor."""
    url_descarga = f"{SERVER_URL}/api/orden/archivo/{id_orden}"
    try:
        r = requests.get(url_descarga, stream=True)
        if r.status_code == 200:
            with open(ruta_guardado, "wb") as f:
                f.write(r.content)
            print(f"      ✅ Descarga completada: {ruta_guardado}")
        else:
            print(f"      ❌ Falló la descarga. Status: {r.status_code}")
    except Exception as e:
        print(f"      ❌ Error escribiendo archivo: {e}")

# -------------------------------
#   FUNCION PRINCIPAL: SINCRONIZAR Y DESCARGAR
# -------------------------------
def sincronizar_y_descargar():
    print(f"\n[SYNC] Consultando servidor: {API_LISTA}")
    try:
        res = requests.get(API_LISTA, timeout=5)
    except Exception as e:
        print(f"[ERROR CONEXION] {e}")
        actualizar_estado_db("ERROR")
        return

    if res.status_code != 200:
        print(f"[ERROR] Servidor respondió: {res.status_code}")
        actualizar_estado_db("ERROR")
        return

    actualizar_estado_db("OK")

    # Convertimos la respuesta a JSON
    try:
        ordenes = res.json()
    except Exception as e:
        print(f"[ERROR] JSON inválido: {e}")
        actualizar_estado_db("ERROR")
        return

    print(f"[SYNC] {len(ordenes)} órdenes encontradas.")

    # Guardar JSON para que el programa en C lo lea
    try:
        with open(JSON_FILE, "w") as f:
            f.write(res.text)
        print(f"[SYNC] Archivo {JSON_FILE} actualizado.")
    except Exception as e:
        print(f"[ERROR] No se pudo escribir {JSON_FILE}: {e}")

    # -------------------------------
    # Descargar solo archivos asignados a esta máquina
    # -------------------------------
    for orden in ordenes:
        if orden.get("maquina") != MI_ID_MAQUINA:
            continue

        archivo_nombre = limpiar_nombre(orden.get("archivo_nombre", "sin_nombre.gcode"))
        ruta_final = os.path.join(CARPETA_DESCARGAS, archivo_nombre)

        if os.path.exists(ruta_final):
            print(f"   ↳ ✅ Archivo ya existente: {archivo_nombre}")
            continue

        print(f"   ↳ 📥 Nuevo archivo detectado: {archivo_nombre}")
        descargar_archivo(orden.get("id"), ruta_final)

# -------------------------------
#   MAIN LOOP
# -------------------------------
if __name__ == "__main__":
    print(f"--- INICIANDO MONITOR DE BASE DE DATOS Y DESCARGAS ({MI_ID_MAQUINA}) ---")
    sincronizar_y_descargar()

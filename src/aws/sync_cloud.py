import requests
import os
import time
import re

# --- CONFIGURACIÓN ---
SERVER_URL = "http://54.123.45.67:3000" 
API_LISTA = f"{SERVER_URL}/api/ordenes"
API_BAJAR = f"{SERVER_URL}/api/download"

LOCAL_DIR = "/home/merry/proyecto_cnc_hmi/gcode_files/"
STATUS_FILE = "/home/merry/proyecto_cnc_hmi/db_status.txt" # <--- NUEVO

def actualizar_estado_db(estado):
    """Escribe OK o ERROR en un archivo para que C lo lea"""
    try:
        with open(STATUS_FILE, "w") as f:
            f.write(estado)
    except:
        pass

def limpiar_nombre(nombre):
    return re.sub(r'[^a-zA-Z0-9_.-]', '_', nombre)

def sincronizar():
    print(f"[SYNC] Consultando {API_LISTA}...")
    try:
        res = requests.get(API_LISTA, timeout=5)
        
        if res.status_code != 200:
            print(f"[ERROR] Servidor respondio: {res.status_code}")
            actualizar_estado_db("ERROR") # <--- REPORTAR ERROR
            return

        # Si llegamos aquí, hay conexión
        actualizar_estado_db("OK") # <--- REPORTAR OK
        
        ordenes = res.json()
        print(f"[SYNC] {len(ordenes)} ordenes encontradas.")

        for orden in ordenes:
            nombre_orig = orden.get('archivo_nombre')
            if not nombre_orig or nombre_orig == 'null': continue

            id_orden = orden.get('id')
            producto = orden.get('producto', 'Producto')
            
            ext = os.path.splitext(nombre_orig)[1] 
            if not ext: ext = ".gcode"
            
            nombre_final = f"{id_orden}_{limpiar_nombre(producto)}{ext}"
            ruta_local = os.path.join(LOCAL_DIR, nombre_final)

            if not os.path.exists(ruta_local):
                print(f"[NUEVO] Descargando: {nombre_final}")
                url_descarga = f"{API_BAJAR}/{id_orden}"
                try:
                    r_file = requests.get(url_descarga, stream=True)
                    if r_file.status_code == 200:
                        with open(ruta_local, 'wb') as f:
                            for chunk in r_file.iter_content(chunk_size=8192):
                                f.write(chunk)
                        print("[OK] Descarga completada.")
                    else:
                        print(f"[FALLO] Error bajando archivo")
                except:
                    print("[FALLO] Error de red al bajar archivo")
            
    except Exception as e:
        print(f"[ERROR CONEXION] {e}")
        actualizar_estado_db("ERROR") # <--- REPORTAR ERROR EXCEPCIÓN

if __name__ == "__main__":
    if not os.path.exists(LOCAL_DIR): os.makedirs(LOCAL_DIR)
    print("--- INICIANDO MONITOR DE BASE DE DATOS ---")
    
    # Estado inicial desconocido
    actualizar_estado_db("ERROR") 
    
    while True:
        sincronizar()
        time.sleep(10)

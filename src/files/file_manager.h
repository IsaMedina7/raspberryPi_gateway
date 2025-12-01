#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

// Definir la ruta donde buscar los archivos .gcode
// Usamos ruta relativa "gcode_files" que debe estar en la raíz del proyecto
#define GCODE_DIR "gcode_files"

// Límites para evitar desbordamientos de memoria
#define MAX_FILES 50          // Máximo número de archivos a listar
#define MAX_FILENAME_LEN 64   // Longitud máxima del nombre (ej: "diseño_final_v2.gcode")

// Estructura para guardar la lista de archivos
typedef struct {
    // Array bidimensional: Una lista de cadenas de texto
    char filenames[MAX_FILES][MAX_FILENAME_LEN];

    // Cantidad real de archivos encontrados
    int count;
} FileList;

// --- PROTOTIPOS DE FUNCIONES ---

/**
 * Escanea el directorio definido en GCODE_DIR y llena la lista.
 * @param list Puntero a la estructura FileList donde se guardarán los nombres.
 */
void fm_scan_directory(FileList *list);

extern FileList mis_archivos;

#endif // FILE_MANAGER_H


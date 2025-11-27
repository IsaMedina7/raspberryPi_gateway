#include "file_manager.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h> // Librería estándar de Linux para directorios

void fm_scan_directory(FileList *list) {
    DIR *d;
    struct dirent *dir;

    // Reiniciar el contador de archivos
    list->count = 0;

    // Intentar abrir el directorio
    d = opendir(GCODE_DIR);

    if (d) {
        printf("[FILE MANAGER] Escaneando carpeta '%s'...\n", GCODE_DIR);

        // Leer cada entrada del directorio
        while ((dir = readdir(d)) != NULL) {

            // 1. Ignorar archivos ocultos (los que empiezan con punto, como . o ..)
            if (dir->d_name[0] == '.') continue;

            // 2. Filtrar por extensión (.gcode o .nc)
            // strstr busca si una cadena está dentro de otra
            if (strstr(dir->d_name, ".gcode") || strstr(dir->d_name, ".nc")) {

                // Verificar que no nos pasemos del límite de nuestro array
                if (list->count < MAX_FILES) {

                    // Copiar el nombre del archivo a nuestra estructura
                    strncpy(list->filenames[list->count], dir->d_name, MAX_FILENAME_LEN - 1);

                    // Asegurar que la cadena termine con el caracter nulo '\0' (seguridad)
                    list->filenames[list->count][MAX_FILENAME_LEN - 1] = '\0';

                    printf("   > Encontrado: %s\n", dir->d_name);

                    // Incrementar contador
                    list->count++;
                } else {
                    printf("[FILE MANAGER WARN] Límite de archivos alcanzado (%d)\n", MAX_FILES);
                    break; // Salir del bucle si ya no caben más
                }
            }
        }
        // Cerrar el directorio al terminar
        closedir(d);
    } else {
        // Si falla (ej. la carpeta no existe)
        printf("[FILE MANAGER ERROR] No se pudo abrir la carpeta '%s'. ¿La creaste?\n", GCODE_DIR);
    }
}


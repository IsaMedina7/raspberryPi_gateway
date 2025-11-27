#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

#define LOG_FILE "events.log"

void logger_init(void) {
    FILE *f = fopen(LOG_FILE, "a"); // 'a' = append (no borrar lo anterior)
    if (f) {
        fprintf(f, "--- INICIO DE SESION ---\n");
        fclose(f);
    }
}

void logger_log(const char* tipo, const char* mensaje) {
    time_t now;
    struct tm *t;
    char fecha[64];

    // Obtener hora actual
    time(&now);
    t = localtime(&now);
    strftime(fecha, sizeof(fecha)-1, "%Y-%m-%d %H:%M:%S", t);

    // Escribir en archivo
    FILE *f = fopen(LOG_FILE, "a");
    if (f) {
        // Formato: [FECHA] [TIPO] Mensaje
        fprintf(f, "[%s] [%s] %s\n", fecha, tipo, mensaje);
        fclose(f);

        // También imprimir en consola para debug
        printf("[LOG] %s: %s\n", tipo, mensaje);
    } else {
        printf("[LOGGER ERROR] No se pudo escribir en el archivo de logs.\n");
    }
}


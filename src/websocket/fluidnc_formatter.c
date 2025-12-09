#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
// #include "fluidnc_commands.h"

// --------------------------------------------------------------------------
// Formateador de movimiento (Jog)
// --------------------------------------------------------------------------
void fluidnc_format_jog(char axis, float distance, int feed_rate, char *cmd) {
    // $J= Inicia comando de Jogging
    // G91: Modo relativo (incremental)
    // G21: Unidades en milímetros
    // %c: El eje (X, Y, Z)
    // %+.3f: Flotante con signo explícito (+ o -) y 3 decimales (Ej: +10.000)
    // F%d: Feed rate (velocidad)
    
    sprintf(cmd, "$J=G91 G21 %c%+.3f F%d", toupper(axis), distance, feed_rate);
}

// --------------------------------------------------------------------------
// Comandos de Control (Home, Stop, Status)
// --------------------------------------------------------------------------
void fluidnc_format_home(char *cmd) {
    // $H ejecuta el ciclo de homing definido en la configuración
    strcpy(cmd, "$H");
}

void fluidnc_format_stop(char *cmd) {
    // ! es el carácter de Feed Hold inmediato en Grbl/FluidNC
    strcpy(cmd, "!");
}

void fluidnc_format_status(char *cmd) {
    // ? solicita el reporte de estado inmediato
    strcpy(cmd, "?");
}

// --------------------------------------------------------------------------
// Parser de Posición (MPos)
// --------------------------------------------------------------------------
int fluidnc_parse_mpos(const char *response, float *x, float *y, float *z) {
    if (!response || !x || !y || !z) return 0;

    const char *ptr = NULL;

    // 1. Intento de parseo Formato Legado (Grbl Style)
    // Ejemplo: <Idle|MPos:10.000,20.000,30.000|FS:0,0>
    ptr = strstr(response, "MPos:");
    if (ptr) {
        // Avanzamos el puntero después de "MPos:"
        ptr += 5; 
        if (sscanf(ptr, "%f,%f,%f", x, y, z) == 3) {
            return 1; // Éxito
        }
    }

    // 2. Intento de parseo Formato JSON (FluidNC moderno)
    // FluidNC a veces devuelve JSON si está configurado así en el canal.
    // Ejemplo simplificado: "MPos":[10.00, 20.00, 30.00] o similar.
    // Buscamos "MPos" seguido de caracteres hasta encontrar un numero o corchete.
    
    // Reiniciamos búsqueda
    ptr = strstr(response, "\"MPos\""); // Busca la clave JSON con comillas
    if (!ptr) ptr = strstr(response, "pos"); // Algunos reportes usan "pos"

    if (ptr) {
        // Buscar el inicio del array o los valores. 
        // Avanzamos hasta encontrar un dígito o un signo menos
        while (*ptr && !isdigit(*ptr) && *ptr != '-') {
            ptr++;
        }
        
        // Parsear formato JSON array [x, y, z] o x,y,z
        // Nota: sscanf maneja espacios en blanco automáticamente
        // El formato string "%f , %f , %f" intenta saltar comas y espacios
        if (*ptr && sscanf(ptr, "%f , %f , %f", x, y, z) == 3) {
            return 1; // Éxito
        }
    }

    return 0; // No se pudo parsear
}
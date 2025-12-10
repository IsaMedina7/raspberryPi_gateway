#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "fluidnc_formatter.h"

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

// --------------------------------------------------------------------------
// Formateador de comando para subir archivos a la ESP (upload)
// Formato esperado:
// "file=@/path/to/local/file.gcode;filename=/test.gcode" http://${FLUIDNC_FQDN}/upload
// --------------------------------------------------------------------------
void fluidnc_format_upload(const char *local_path, const char *remote_filename, char *cmd) {
    if (!local_path || !remote_filename || !cmd) return;

    // Asegurarnos de que el nombre remoto comienza con '/'
    char remote[256];
    if (remote_filename[0] == '/') {
        snprintf(remote, sizeof(remote), "%s", remote_filename);
    } else {
        snprintf(remote, sizeof(remote), "/%s", remote_filename);
    }

    // Formateamos la cadena según el ejemplo. Usamos FLUIDNC_CMD_MAX como tamaño recomendado.
    // Nota: si la ruta local es muy larga puede truncarse según el tamaño del buffer de salida.
    snprintf(cmd, FLUIDNC_CMD_MAX, "\"file=@%s;filename=%s\" http://${FLUIDNC_FQDN}/upload", local_path, remote);
}

// Función simple para subir archivo usando curl via system call
int upload_file_to_sd(const char *ip, const char *local_path, const char *sd_filename) {
    char cmd_buffer[2048];
    
    // Construimos el comando curl exacto que vimos antes
    // NOTA: Asumimos puerto 80 para HTTP por defecto, o la IP debe incluirlo si es distinto
    // En tu caso la IP tiene puerto 81, así que lo manejamos en el formato
    int ret = snprintf(cmd_buffer, sizeof(cmd_buffer), 
             "curl -s -F \"file=@%s;filename=%s\" http://%s/upload", 
             local_path, sd_filename, ip);

    if (ret >= sizeof(cmd_buffer)) {
        fprintf(stderr, "ERROR: Rutas demasiado largas para el buffer.\n");
        return 1;
    }

    printf("Subiendo archivo a SD... Espere.\n");
    // system devuelve el estado de salida del comando (0 es éxito en shell)
    int status = system(cmd_buffer);
    
    if (status == 0) {
        printf("\n[Exito] Archivo subido correctamente.\n");
        return 0;
    } else {
        printf("\n[Error] Falló la subida (curl exit code: %d).\n", status);
        return 1;
    }
}

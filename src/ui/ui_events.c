#include "ui.h"
#include "screens/ui_seleccionarTarea.h"
#include "../mqtt/mqtt_service.h"
#include "../files/file_manager.h"
#include "../logger/logger.h"
#include "ui_logic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Variables Globales
int maquina_activa_id = 1;
//extern FileList mis_archivos;

// --- HELPER ---
void enviar_orden_cnc(const char* comando) {
    char topico_id[32];
    sprintf(topico_id, "maquina_%d", maquina_activa_id);

    // 1. Enviar por MQTT (Real)
    mqtt_send_command(topico_id, comando);

    // 2. MOSTRAR EN PANTALLA (TEXT AREA)
    char log_visual[128];
    // Formato: ">> JOG:X:10"
    snprintf(log_visual, sizeof(log_visual), ">> %s", comando);
    ui_add_log(log_visual);

    // 3. Log interno del sistema
    printf("[DEBUG TX] %s\n", log_visual);
}

// ========================================================
// 1. LÓGICA DE ARCHIVOS (ROLLER)
// ========================================================

// Función para llenar el ROLLER de forma segura
void RefrescarListaArchivos(lv_event_t * e) {
    lv_obj_t * roller = ui_listaTareas1;

    if (!roller) {
        printf("[UI ERROR] El objeto Roller no existe aún.\n");
        return;
    }

    // 1. Escanear archivos
    fm_scan_directory(&mis_archivos);

    if (mis_archivos.count == 0) {
        lv_roller_set_options(roller, "Sin archivos", LV_ROLLER_MODE_NORMAL);
        return;
    }

    // 2. Construir la cadena gigante de forma segura (Memoria Dinámica)
    // Calculamos el tamaño necesario aprox: 64 chars por archivo * cantidad
    size_t buffer_size = mis_archivos.count * 65;
    char *opciones = (char*)malloc(buffer_size); // Pedimos memoria al sistema

    if (opciones == NULL) {
        printf("[UI CRITICO] No hay memoria para listar archivos.\n");
        return;
    }

    // Limpiamos el buffer
    opciones[0] = '\0';

    for(int i=0; i < mis_archivos.count; i++) {
        strcat(opciones, mis_archivos.filenames[i]);

        // Agregar salto de línea si no es el último
        if (i < mis_archivos.count - 1) {
            strcat(opciones, "\n");
        }
    }

    // 3. Asignar al Roller
    lv_roller_set_options(roller, opciones, LV_ROLLER_MODE_NORMAL);

    // 4. Liberar la memoria temporal (¡Muy importante!)
    free(opciones);

    printf("[UI] Roller actualizado con %d archivos.\n", mis_archivos.count);
}

// Botón "Asignar" (Enviar selección del Roller)
void EnviarArchivoDesdeRoller(lv_event_t * e) {
    lv_obj_t * roller = ui_listaTareas1;
    char seleccion[128];

    // Leer qué eligió el usuario
    lv_roller_get_selected_str(roller, seleccion, sizeof(seleccion));

    // Validar
    if (strlen(seleccion) == 0 || strcmp(seleccion, "Sin archivos") == 0) {
        printf("[UI] Selección inválida.\n");
        return;
    }

    printf("[UI] Asignando %s a Maquina %d\n", seleccion, maquina_activa_id);

    char comando[256];
    snprintf(comando, sizeof(comando), "DOWNLOAD:%s", seleccion);
    enviar_orden_cnc(comando);

    // Regresar al inicio
    retrocederMain(NULL);
}

// Botón "+" del Dashboard
void IrSeleccionarTarea(lv_event_t * e) {
    printf("[UI] Abriendo selector...\n");

    // Cambiar pantalla primero
    if (ui_seleccionarTarea) {
        _ui_screen_change(&ui_seleccionarTarea, LV_SCR_LOAD_ANIM_NONE, 0, 0, &ui_seleccionarTarea_screen_init);
    } else {
        ui_seleccionarTarea_screen_init();
        lv_scr_load(ui_seleccionarTarea);
    }

    // Llenar la lista después de cargar la pantalla
    // (Pequeño delay técnico para asegurar que el objeto exista)
    RefrescarListaArchivos(NULL);
}

// Botón Atrás
void retrocederMain(lv_event_t * e) {
    _ui_screen_change(&ui_main, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, &ui_main_screen_init);
}

// ========================================================
// 2. CONTROL (MOVIMIENTO)
// ========================================================

void SelectorMaquina(lv_event_t * e) {
    lv_obj_t * obj = lv_event_get_target(e);
    int index = lv_roller_get_selected(obj);
    maquina_activa_id = index + 1;
    char buf[32]; snprintf(buf, 32, "Controlando: M%d", maquina_activa_id);
    ui_add_log(buf);
}

void JogXPlus(lv_event_t * e) { enviar_orden_cnc("JOG:X:10"); }
void JogXMinus(lv_event_t * e) { enviar_orden_cnc("JOG:X:-10"); }
void JogYPlus(lv_event_t * e) { enviar_orden_cnc("JOG:Y:10"); }
void JogYMinus(lv_event_t * e) { enviar_orden_cnc("JOG:Y:-10"); }
void JogZPlus(lv_event_t * e) { enviar_orden_cnc("JOG:Z:5"); }
void JogZMinus(lv_event_t * e) { enviar_orden_cnc("JOG:Z:-5"); }

void CmdHome(lv_event_t * e) { enviar_orden_cnc("HOME"); }
void CmdStop(lv_event_t * e) { enviar_orden_cnc("STOP"); }

void EmergenciaTotal(lv_event_t * e) {
    mqtt_send_command("todas", "EMERGENCIA");
    logger_log("CRITICO", "PARO GENERAL ACTIVADO");
    ui_add_log("!!! PARO GENERAL !!!");
}

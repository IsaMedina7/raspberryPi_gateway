#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "lvgl.h"
#include "sdl/sdl.h"
#include "mqtt/mqtt_service.h"
#include "files/file_manager.h"
#include "logger/logger.h"
#include "ui/ui.h"
#include "ui/ui_logic.h" // Necesario para ui_set_connection_status

// --- VARIABLES GLOBALES ---
SystemState global_state;
pthread_mutex_t state_mutex;
lv_obj_t * cursor_obj;

// Variable externa de conexión (viene de mqtt_service.c)
extern int mqtt_conectado;

// --- OBJETOS UI EXTERNOS (Solo si necesitas forzar conexiones manuales) ---
extern lv_obj_t * ui_agregarTareas;
extern lv_obj_t * ui_btnAtras;
extern lv_obj_t * ui_asignarTarea;
extern lv_obj_t * ui_areaComands;

// --- FUNCIONES EXTERNAS ---
extern void IrSeleccionarTarea(lv_event_t * e);
extern void retrocederMain(lv_event_t * e);
extern void EnviarArchivoDesdeRoller(lv_event_t * e);

// --- HARDWARE INIT ---
void hal_init(void) {
    sdl_init();

    // Buffer de dibujo
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf[SDL_HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, SDL_HOR_RES * 100);

    // Driver de Pantalla
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;
    lv_disp_drv_register(&disp_drv);

    // Driver de Mouse (Input)
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv);

    // Cursor Visual (+)
    cursor_obj = lv_label_create(lv_scr_act());
    lv_label_set_text(cursor_obj, "+");
    lv_indev_set_cursor(mouse_indev, cursor_obj);
}

// Función para guardar estado en JSON (Para AWS)
void exportar_estado_json() {
    FILE *f = fopen("state.json", "w");
    if (f) { fprintf(f, "{\"ts\": %ld}", time(NULL)); fclose(f); }
}

// --- HILO UI (LA INTERFAZ GRÁFICA) ---
void* thread_ui_loop(void* arg) {
    lv_init();      // 1. Iniciar LVGL Core
    hal_init();     // 2. Iniciar Drivers Hardware

    printf("[UI] Iniciando interfaz SquareLine...\n");
    ui_init();      // 3. Cargar diseño exportado

    // --- CONFIGURACIONES ADICIONALES (POST-INIT) ---

    // A. Configurar Consola como Solo Lectura
    if (ui_areaComands) {
        printf("[UI] Configurando consola como Solo Lectura...\n");
        lv_obj_clear_flag(ui_areaComands, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_flag(ui_areaComands, LV_OBJ_FLAG_SCROLLABLE);
    }

    // B. Parche de Seguridad: Conectar Botones Manualmente
    // (Si ya configuraste los eventos en SquareLine, puedes comentar esto)
    if (ui_agregarTareas) {
        lv_obj_add_flag(ui_agregarTareas, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_event_cb(ui_agregarTareas, NULL);
        lv_obj_add_event_cb(ui_agregarTareas, IrSeleccionarTarea, LV_EVENT_CLICKED, NULL);
    }

    if (ui_btnAtras) {
        lv_obj_add_flag(ui_btnAtras, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_event_cb(ui_btnAtras, NULL);
        lv_obj_add_event_cb(ui_btnAtras, retrocederMain, LV_EVENT_CLICKED, NULL);
    }

    if (ui_asignarTarea) {
        lv_obj_add_flag(ui_asignarTarea, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_event_cb(ui_asignarTarea, NULL);
        lv_obj_add_event_cb(ui_asignarTarea, EnviarArchivoDesdeRoller, LV_EVENT_CLICKED, NULL);
    }
    // --------------------------------------------------------

    // Variable para detectar cambios en la conexión
    int ultimo_estado_conn = -1;

    // BUCLE INFINITO DE LA UI
    while(1) {
        // Tarea gráfica (vital)
        lv_timer_handler();

        // 1. Actualizar Semáforo de Conexión
        if (mqtt_conectado != ultimo_estado_conn) {
            ui_set_connection_status(1, mqtt_conectado);
            ultimo_estado_conn = mqtt_conectado;

            // Log interno
            if(mqtt_conectado) logger_log("INFO", "Conexion MQTT Restaurada");
            else logger_log("ALERTA", "Conexion MQTT Perdida");
        }

        // 2. Revisar actualizaciones de Máquinas
        pthread_mutex_lock(&state_mutex);
        if (global_state.hay_actualizacion) {
            exportar_estado_json(); // Generar JSON para la nube
            global_state.hay_actualizacion = 0;
        }
        pthread_mutex_unlock(&state_mutex);

        // Dormir 5ms para liberar CPU
        usleep(5000);
    }
    return NULL;
}

// --- PUNTO DE ENTRADA ---
int main() {
    // 1. Iniciar Logger
    logger_init();

    printf("--- CNC GATEWAY HMI INICIANDO ---\n");

    // 2. Iniciar Estructuras de Datos
    pthread_mutex_init(&state_mutex, NULL);
    memset(&global_state, 0, sizeof(SystemState));

    // 3. Lanzar Hilos
    pthread_t t_ui, t_mqtt;

    // Hilo de Comunicación (MQTT)
    if(pthread_create(&t_mqtt, NULL, thread_mqtt_loop, NULL) != 0) {
        printf("Error creando hilo MQTT\n");
        return 1;
    }

    // Hilo de Interfaz (UI)
    if(pthread_create(&t_ui, NULL, thread_ui_loop, NULL) != 0) {
        printf("Error creando hilo UI\n");
        return 1;
    }

    // 4. Esperar (Nunca termina)
    pthread_join(t_mqtt, NULL);
    pthread_join(t_ui, NULL);

    return 0;
}

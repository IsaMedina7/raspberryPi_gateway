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
#include "ui/ui_logic.h"

// --- NO AWS ---

SystemState global_state;
pthread_mutex_t state_mutex;
lv_obj_t * cursor_obj;
extern int mqtt_conectado;

// Externos UI
extern lv_obj_t * ui_agregarTareas;
extern lv_obj_t * ui_btnAtras;
extern lv_obj_t * ui_asignarTarea;
extern lv_obj_t * ui_areaComands;

// Funciones Externas (Coinciden con ui_events.c)
extern void IrSeleccionarTarea(lv_event_t * e);
extern void retrocederMain(lv_event_t * e);
extern void EnviarArchivoDesdeRoller(lv_event_t * e);
extern void agregar_tarea(lv_event_t * e); // Usamos el nombre de SquareLine
extern void asignar_tarea(lv_event_t * e); // Usamos el nombre de SquareLine

void hal_init(void) {
    sdl_init();
    static lv_disp_draw_buf_t disp_buf;
    static lv_color_t buf[SDL_HOR_RES * 100];
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, SDL_HOR_RES * 100);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &disp_buf;
    disp_drv.flush_cb = sdl_display_flush;
    disp_drv.hor_res = SDL_HOR_RES;
    disp_drv.ver_res = SDL_VER_RES;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_read;
    lv_indev_t * mouse_indev = lv_indev_drv_register(&indev_drv);

    cursor_obj = lv_label_create(lv_scr_act());
    lv_label_set_text(cursor_obj, "+");
    lv_indev_set_cursor(mouse_indev, cursor_obj);
}

void exportar_estado_json() {
    FILE *f = fopen("state.json", "w");
    if (f) { fprintf(f, "{\"ts\": %ld}", time(NULL)); fclose(f); }
}

void* thread_ui_loop(void* arg) {
    lv_init();
    hal_init();
    ui_init();
    ui_init_custom_label();

    if (ui_areaComands) {
        lv_obj_clear_flag(ui_areaComands, LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_add_flag(ui_areaComands, LV_OBJ_FLAG_SCROLLABLE);
    }

    // --- PARCHES MANUALES (Usamos las funciones wrapper) ---
    if (ui_agregarTareas) {
        lv_obj_add_flag(ui_agregarTareas, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_event_cb(ui_agregarTareas, NULL);
        lv_obj_add_event_cb(ui_agregarTareas, agregar_tarea, LV_EVENT_CLICKED, NULL);
    }
    if (ui_btnAtras) {
        lv_obj_add_flag(ui_btnAtras, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_event_cb(ui_btnAtras, NULL);
        lv_obj_add_event_cb(ui_btnAtras, retrocederMain, LV_EVENT_CLICKED, NULL);
    }
    if (ui_asignarTarea) {
        lv_obj_add_flag(ui_asignarTarea, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_event_cb(ui_asignarTarea, NULL);
        lv_obj_add_event_cb(ui_asignarTarea, asignar_tarea, LV_EVENT_CLICKED, NULL);
    }
    // -----------------------------------------------------

    int ultimo_conn = -1;
    while(1) {
        lv_timer_handler();
        if (mqtt_conectado != ultimo_conn) {
            ui_set_connection_status(1, mqtt_conectado);
            ultimo_conn = mqtt_conectado;
        }
        pthread_mutex_lock(&state_mutex);
        if (global_state.hay_actualizacion) {
            exportar_estado_json();
            global_state.hay_actualizacion = 0;
        }
        pthread_mutex_unlock(&state_mutex);
        usleep(5000);
    }
    return NULL;
}

int main() {
    logger_init();
    pthread_mutex_init(&state_mutex, NULL);
    memset(&global_state, 0, sizeof(SystemState));

    pthread_t t_ui, t_mqtt;

    pthread_create(&t_mqtt, NULL, thread_mqtt_loop, NULL);
    pthread_create(&t_ui, NULL, thread_ui_loop, NULL);

    // SIN HILO AWS

    pthread_join(t_mqtt, NULL);
    pthread_join(t_ui, NULL);
    return 0;
}

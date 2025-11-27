#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "lvgl.h"
#include "sdl/sdl.h"
#include "mqtt/mqtt_service.h"
#include "logger/logger.h"
#include "ui/ui_manager.h"

SystemState global_state;
pthread_mutex_t state_mutex;
lv_obj_t * cursor_obj;

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

void* thread_ui_loop(void* arg) {
    lv_init();
    hal_init();

    printf("[UI] Iniciando Panel CNC 800x480...\n");
    ui_init(); // Cargar la interfaz nueva

    while(1) {
        lv_timer_handler();

        // Revisar actualizaciones del Backend (MQTT)
        pthread_mutex_lock(&state_mutex);
        if (global_state.hay_actualizacion) {
            int idx = global_state.ultima_maquina_actualizada_id - 1;

            if(idx >= 0 && idx < MAX_MAQUINAS) {
                // 1. Actualizar Estado (Texto y Color)
                ui_update_status(global_state.maquinas[idx].estado);

                // 2. Actualizar Coordenadas (Si llegan en el mensaje)
                ui_update_coords(global_state.maquinas[idx].pos_x,
                                 global_state.maquinas[idx].pos_y,
                                 global_state.maquinas[idx].pos_z);

                // 3. Log interno
                logger_log("INFO", "Actualizacion recibida");
            }
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

    pthread_join(t_mqtt, NULL);
    pthread_join(t_ui, NULL);
    return 0;
}

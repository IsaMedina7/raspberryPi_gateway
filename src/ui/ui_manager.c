#include "ui_manager.h"
#include <stdio.h>
#include "../mqtt/mqtt_service.h"
#include "../logger/logger.h"

// Variables Globales
static lv_obj_t * label_pos_x;
static lv_obj_t * label_pos_y;
static lv_obj_t * label_pos_z;
static lv_obj_t * label_status;
static lv_obj_t * textarea_log;
static int maquina_id_actual = 1;

// Estilos
static lv_style_t style_panel;
static lv_style_t style_btn_move;
static lv_style_t style_btn_stop;

void init_styles() {
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, lv_palette_darken(LV_PALETTE_GREY, 3));
    lv_style_set_radius(&style_panel, 10);
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_border_color(&style_panel, lv_palette_main(LV_PALETTE_GREY));

    lv_style_init(&style_btn_move);
    lv_style_set_bg_color(&style_btn_move, lv_palette_main(LV_PALETTE_BLUE_GREY));
    lv_style_set_radius(&style_btn_move, 50);

    lv_style_init(&style_btn_stop);
    lv_style_set_bg_color(&style_btn_stop, lv_palette_main(LV_PALETTE_RED));
}

// --- LÓGICA DE EVENTOS ---

void enviar_cmd(const char* cmd) {
    char topic[32];
    snprintf(topic, sizeof(topic), "maquina_%d", maquina_id_actual);
    mqtt_send_command(topic, cmd);
    char buf[64]; snprintf(buf, 64, "TX: %s", cmd);
    ui_add_log(buf);
}

static void event_jog(lv_event_t * e) {
    const char* eje_dir = (const char*)lv_event_get_user_data(e);
    char eje = eje_dir[0];
    char dir = eje_dir[1];
    float distancia = (dir == '+') ? 10.0 : -10.0;
    if (eje == 'Z') distancia = (dir == '+') ? 5.0 : -5.0;

    char cmd[32];
    snprintf(cmd, sizeof(cmd), "JOG:%c:%.2f", eje, distancia);
    enviar_cmd(cmd);
}

static void event_emergencia(lv_event_t * e) {
    mqtt_send_command("todas", "EMERGENCIA");
    ui_add_log("!!! PARO GENERAL !!!");
}

static void event_cambio_maquina(lv_event_t * e) {
    lv_obj_t * dd = lv_event_get_target(e);
    maquina_id_actual = lv_dropdown_get_selected(dd) + 1;
    char buf[32]; snprintf(buf, 32, "Seleccionada: M%d", maquina_id_actual);
    ui_add_log(buf);
}

// --- UI INIT ---
void ui_init(void) {
    init_styles();
    lv_obj_t * scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x101010), 0);

    // HEADER
    lv_obj_t * header = lv_obj_create(scr);
    lv_obj_set_size(header, 800, 60);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x303030), 0);

    lv_label_set_text(lv_label_create(header), "CONTROL CENTRAL CNC");

    lv_obj_t * btn_emg = lv_btn_create(header);
    lv_obj_align(btn_emg, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_style(btn_emg, &style_btn_stop, 0);
    lv_obj_add_event_cb(btn_emg, event_emergencia, LV_EVENT_CLICKED, NULL);
    lv_label_set_text(lv_label_create(btn_emg), "PARO TOTAL");

    // PANEL IZQUIERDO
    lv_obj_t * p_left = lv_obj_create(scr);
    lv_obj_set_size(p_left, 250, 400);
    lv_obj_align(p_left, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(p_left, &style_panel, 0);

    lv_obj_t * dd = lv_dropdown_create(p_left);
    lv_dropdown_set_options(dd, "Maquina 1\nMaquina 2\nMaquina 3");
    lv_obj_set_width(dd, 200);
    lv_obj_add_event_cb(dd, event_cambio_maquina, LV_EVENT_VALUE_CHANGED, NULL);

    textarea_log = lv_textarea_create(p_left);
    lv_obj_set_size(textarea_log, 210, 200);
    lv_obj_align(textarea_log, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_textarea_set_text(textarea_log, "Sistema listo...\n");

    // --- CORRECCIÓN FINAL AQUÍ ---
    // Usamos clear_flag en lugar de la función que no existía
    lv_obj_clear_flag(textarea_log, LV_OBJ_FLAG_CLICKABLE);

    // PANEL DERECHO (Solo ejemplo botones básicos para verificar compilación)
    lv_obj_t * p_right = lv_obj_create(scr);
    lv_obj_set_size(p_right, 520, 400);
    lv_obj_align(p_right, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(p_right, &style_panel, 0);

    lv_obj_t * btn_xp = lv_btn_create(p_right);
    lv_obj_align(btn_xp, LV_ALIGN_CENTER, 60, 0);
    lv_label_set_text(lv_label_create(btn_xp), "X+");
    lv_obj_add_event_cb(btn_xp, event_jog, LV_EVENT_CLICKED, "X+");
}

// --- FUNCIONES PÚBLICAS ---
void ui_update_coords(float x, float y, float z) { /* ... */ }
void ui_update_status(const char* estado) { /* ... */ }
void ui_add_log(const char* mensaje) {
    if(textarea_log) lv_textarea_add_text(textarea_log, mensaje);
    if(textarea_log) lv_textarea_add_text(textarea_log, "\n");
}

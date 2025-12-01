#include "ui.h"
#include <stdio.h>
#include <string.h>

// --- ACTUALIZAR CONEXIÓN (USANDO TU LABEL REAL) ---
void ui_set_connection_status(int tipo, int estado) {
    // Buscamos el objeto que creaste en SquareLine.
    // Asegúrate de que en ui_main.h exista 'extern lv_obj_t * ui_LabelStatusConn;'
    // Si le pusiste otro nombre, cámbialo aquí.
    lv_obj_t * label = ui_uiLabelStatusConn;

    if (!label) return;

    if (estado == 1) {
        lv_label_set_text(label, "MQTT: OK");
        lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_GREEN), 0);
    } else {
        lv_label_set_text(label, "MQTT: OFF");
        lv_obj_set_style_text_color(label, lv_palette_main(LV_PALETTE_RED), 0);
    }
}

// --- TUS OTRAS FUNCIONES (SE MANTIENEN IGUAL) ---

void ui_update_status(const char* estado) {
    if (ui_Label12) {
        lv_label_set_text(ui_Label12, estado);
        if (strstr(estado, "TRABAJANDO"))
            lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_GREEN), 0);
        else if (strstr(estado, "ERROR"))
            lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_RED), 0);
        else
            lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

void ui_update_coords(float x, float y, float z) {
    if (ui_lblposx) lv_label_set_text_fmt(ui_lblposx, "%.2f", x);
    if (ui_lblposy) lv_label_set_text_fmt(ui_lblposy, "%.2f", y);
    if (ui_lblposz) lv_label_set_text_fmt(ui_lblposz, "%.2f", z);
}

void ui_add_log(const char* mensaje) {
    if (ui_areaComands) {
        lv_textarea_add_text(ui_areaComands, mensaje);
        lv_textarea_add_text(ui_areaComands, "\n");
    }
}

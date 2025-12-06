#include "ui.h"
#include "ui_logic.h"
#include <stdio.h>
#include <string.h>

void ui_update_status(const char* estado) {
    // Etiqueta de estado en panel derecho
    if (ui_Label12) {
        lv_label_set_text(ui_Label12, estado);
        if (strstr(estado, "TRABAJANDO")) lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_GREEN), 0);
        else if (strstr(estado, "ERROR")) lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_RED), 0);
        else lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_GREY), 0);
    }
    // Etiqueta pequeña en la lista de máquinas (si la usas)
    if (ui_lblState) {
        lv_label_set_text(ui_lblState, estado);
    }
}

void ui_update_coords(float x, float y, float z) {
    if (ui_lblposx) lv_label_set_text_fmt(ui_lblposx, "X: %.2f", x);
    if (ui_lblposy) lv_label_set_text_fmt(ui_lblposy, "Y: %.2f", y);
    if (ui_lblposz) lv_label_set_text_fmt(ui_lblposz, "Z: %.2f", z);
}

void ui_add_log(const char* mensaje) {
    if (ui_areaComands) {
        lv_textarea_add_text(ui_areaComands, mensaje);
        lv_textarea_add_text(ui_areaComands, "\n");
    }
}

void ui_set_connection_status(int tipo, int estado) {
    if (ui_uiLabelStatusConn) { // <--- Nombre real de tu captura
        if (estado == 1) {
            lv_label_set_text(ui_uiLabelStatusConn, "ONLINE");
            lv_obj_set_style_text_color(ui_uiLabelStatusConn, lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_label_set_text(ui_uiLabelStatusConn, "OFFLINE");
            lv_obj_set_style_text_color(ui_uiLabelStatusConn, lv_palette_main(LV_PALETTE_RED), 0);
        }
    }
}

void ui_init_custom_label(void) {} // Ya no se usa, pero la dejamos para compatibilidad

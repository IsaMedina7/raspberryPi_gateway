#include "ui.h"
#include "ui_logic.h"
#include <stdio.h>
#include <string.h>

// --- ACTUALIZAR ESTADO (Verde/Rojo) ---
void ui_update_status(const char* estado) {
    if (ui_lblState) {
        lv_label_set_text(ui_lblState, estado);
        if (strstr(estado, "TRABAJANDO"))
            lv_obj_set_style_text_color(ui_lblState, lv_palette_main(LV_PALETTE_GREEN), 0);
        else if (strstr(estado, "ERROR"))
            lv_obj_set_style_text_color(ui_lblState, lv_palette_main(LV_PALETTE_RED), 0);
        else
            lv_obj_set_style_text_color(ui_lblState, lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

// --- ACTUALIZAR COORDENADAS ---
void ui_update_coords(float x, float y, float z) {
    if (ui_lblposx) lv_label_set_text_fmt(ui_lblposx, "X: %.2f", x);
    if (ui_lblposy) lv_label_set_text_fmt(ui_lblposy, "Y: %.2f", y);
    if (ui_lblposz) lv_label_set_text_fmt(ui_lblposz, "Z: %.2f", z);
}

// --- LOGS EN PANTALLA ---
void ui_add_log(const char* mensaje) {
    if (ui_areaComands) {
        lv_textarea_add_text(ui_areaComands, mensaje);
        lv_textarea_add_text(ui_areaComands, "\n");
    }
}

// --- SEMÁFORO DE CONEXIÓN ---
void ui_set_connection_status(int tipo, int estado) {
    if (ui_uiLabelStatusConn) {
        if (estado == 1) {
            lv_label_set_text(ui_uiLabelStatusConn, "ONLINE");
            lv_obj_set_style_text_color(ui_uiLabelStatusConn, lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_label_set_text(ui_uiLabelStatusConn, "OFFLINE");
            lv_obj_set_style_text_color(ui_uiLabelStatusConn, lv_palette_main(LV_PALETTE_RED), 0);
        }
    }
}

void ui_init_custom_label(void) {}

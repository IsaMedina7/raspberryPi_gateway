#include "ui.h"
#include <stdio.h>
#include <string.h>

// --- ACTUALIZAR ESTADO ---
void ui_update_status(const char* estado) {
    // Asumimos que tu etiqueta de estado es ui_Label12 (según vimos antes).
    // Si te da error aquí, revisa ui_main.h de nuevo.
    if (ui_Label12) {
        lv_label_set_text(ui_Label12, estado);

        // Colores básicos
        if (strstr(estado, "TRABAJANDO"))
            lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_GREEN), 0);
        else if (strstr(estado, "ERROR"))
            lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_RED), 0);
        else
            lv_obj_set_style_text_color(ui_Label12, lv_palette_main(LV_PALETTE_GREY), 0);
    }
}

// --- ACTUALIZAR COORDENADAS ---
void ui_update_coords(float x, float y, float z) {
    // Usamos los nombres que vimos en tu ui_main.h
    if (ui_lblposx) lv_label_set_text_fmt(ui_lblposx, "%.2f", x);
    if (ui_lblposy) lv_label_set_text_fmt(ui_lblposy, "%.2f", y);
    if (ui_lblposz) lv_label_set_text_fmt(ui_lblposz, "%.2f", z);
}

// --- LOGS (MODIFICADO PARA NO FALLAR) ---
void ui_add_log(const char* mensaje) {
    // Como no tienes un TextArea en la pantalla,
    // simplemente imprimimos el mensaje en la terminal SSH.
    printf("[PANTALLA LOG] %s\n", mensaje);

    /* NOTA: Si en el futuro agregas un TextArea en SquareLine,
       descomenta esto y pon el nombre correcto:

       if (ui_TuTextArea) {
           lv_textarea_add_text(ui_TuTextArea, mensaje);
           lv_textarea_add_text(ui_TuTextArea, "\n");
       }
    */
}

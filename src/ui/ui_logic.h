#ifndef UI_LOGIC_H
#define UI_LOGIC_H

#include <stdint.h> // Necesario para tipos

// Funciones existentes
void ui_update_status(const char* estado);
void ui_update_coords(float x, float y, float z);
void ui_add_log(const char* mensaje);

// --- NUEVAS FUNCIONES DE CONEXIÓN ---
void ui_init_connection_label(void); // Crear la etiqueta
void ui_set_connection_status(int tipo, int estado); // Cambiar color

#endif

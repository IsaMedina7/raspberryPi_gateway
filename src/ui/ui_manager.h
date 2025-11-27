#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "lvgl.h" // Importación estándar (gracias a CMake include_directories)

// --- OBJETOS ACCESIBLES ---
extern lv_obj_t * label_estado;
extern lv_obj_t * list_files;

// --- FUNCIONES ---
void ui_init(void);
void ui_update_machine_status(int id, const char* estado);
void ui_refresh_file_list(void);
void ui_update_coords(float x, float y, float z);
void ui_update_status(const char* estado);
void ui_add_log(const char* mensaje);

#endif

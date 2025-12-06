#ifndef UI_LOGIC_H
#define UI_LOGIC_H
#include <stdint.h>

void ui_update_status(const char* estado);
void ui_update_coords(float x, float y, float z);
void ui_add_log(const char* mensaje);
void ui_set_connection_status(int tipo, int estado);
void ui_init_custom_label(void);
#endif

/**
 * Configuración MAESTRA - Proyecto CNC Gateway
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_CONF_SKIP 0  // <--- ESTO ENCIENDE LA CONFIGURACIÓN

// Pantalla
#define LV_COLOR_DEPTH 32
#define LV_COLOR_16_SWAP 0
#define LV_DPI_DEF 145  // Ajuste para pantalla de 7 pulgadas

// Memoria
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (4096 * 1024U) // <--- AUMENTADO A 4MB (Antes 128KB)

// Funcionalidades
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
#define LV_USE_USER_DATA 1

// Fuentes
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1 
#define LV_FONT_MONTSERRAT_28 1

// Tema
#define LV_USE_THEME_DEFAULT 1

// Widgets (TODOS ACTIVADOS)
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_BAR 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TABLE 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_LIST 1
#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 1

#endif /*LV_CONF_H*/

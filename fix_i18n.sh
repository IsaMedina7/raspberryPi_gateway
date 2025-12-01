#!/bin/bash

echo "--- CREANDO ARCHIVOS DUMMY DE IDIOMA ---"

# 1. Crear el header (.h) para que ui.h deje de llorar
cat > src/ui/lv_i18n.h <<EOF
#ifndef LV_I18N_H
#define LV_I18N_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Definiciones falsas para satisfacer al compilador
typedef void lv_i18n_language_pack_t;

// Funciones vacías que devuelven 0 (éxito)
static inline int lv_i18n_init(const lv_i18n_language_pack_t * langs) { return 0; }
static inline int lv_i18n_set_locale(const char * locale_name) { return 0; }

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_I18N_H */
EOF

# 2. Crear el source (.c) vacío por si acaso CMake lo busca
cat > src/ui/lv_i18n.c <<EOF
#include "lv_i18n.h"
// Archivo vacío intencional
EOF

echo "--- PARCHE APLICADO ---"

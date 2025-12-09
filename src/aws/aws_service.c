#include "aws_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>
#include "../logger/logger.h"
#include "../mqtt/mqtt_service.h" // Necesitamos acceso al estado global

extern SystemState global_state;
extern pthread_mutex_t state_mutex;

static int force_update_flag = 0;

void aws_trigger_update(void) {
    force_update_flag = 1;
}

// Función para enviar el estado de una máquina
void reportar_maquina(int id, const char* estado) {
    CURL *curl;
    CURLcode res;

    curl = curl_easy_init();
    if(curl) {
        char json_data[128];
        // Construir JSON: {"id": 1, "estado": "TRABAJANDO"}
        snprintf(json_data, sizeof(json_data), "{\"id\": %d, \"estado\": \"%s\"}", id, estado);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, AWS_ENDPOINT_STATUS);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L); // Timeout corto

        // Enviar
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            // printf("[AWS ERROR] No se pudo reportar estado M%d: %s\n", id, curl_easy_strerror(res));
        } else {
            // printf("[AWS] Estado M%d -> %s reportado.\n", id, estado);
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

void* thread_aws_loop(void* arg) {
    printf("[AWS] Hilo de Nube Iniciado.\n");
    curl_global_init(CURL_GLOBAL_ALL);

    while(1) {
        // Dormir pero revisar el flag de forzado cada 100ms
        for(int i=0; i < AWS_STATUS_INTERVAL * 10; i++) {
            if (force_update_flag) {
                force_update_flag = 0;
                break;
            }
            usleep(100000);
        }

        // Reportar estado de TODAS las máquinas activas
        pthread_mutex_lock(&state_mutex);
        for(int i=0; i < MAX_MAQUINAS; i++) {
            if (global_state.maquinas[i].activa) {
                reportar_maquina(global_state.maquinas[i].id, global_state.maquinas[i].estado);
            }
        }
        pthread_mutex_unlock(&state_mutex);
    }

    curl_global_cleanup();
    return NULL;
}

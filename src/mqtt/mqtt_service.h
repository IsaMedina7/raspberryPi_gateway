#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <pthread.h>

#define MAX_MAQUINAS 10

typedef struct {
    int id;
    char estado[32]; // IDLE, TRABAJANDO
    char ip[32];     // <--- NUEVO: IP para WebSocket (ej: "192.168.1.50")
    float pos_x, pos_y, pos_z;
    int activa;      // 1 si está conectada
} MaquinaData;

typedef struct {
    MaquinaData maquinas[MAX_MAQUINAS];
    int hay_actualizacion;
    int ultima_maquina_actualizada_id;
    int lista_cambio; // <--- NUEVO: Bandera para saber si hay máquinas nuevas
} SystemState;

extern SystemState global_state;
extern int mqtt_conectado;

void* thread_mqtt_loop(void* arg);
// Ya no usamos mqtt_send_command para control, solo para estado/discovery si es necesario

#endif

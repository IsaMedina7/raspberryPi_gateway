#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <pthread.h>

#define MAX_MAQUINAS 10

typedef struct {
    int id;
    char estado[32];
    float pos_x;
    float pos_y;
    float pos_z;
    int activa;
} MaquinaData;

typedef struct {
    MaquinaData maquinas[MAX_MAQUINAS];
    int hay_actualizacion;
    int ultima_maquina_actualizada_id;
} SystemState;

// Variable global de conexión
extern int mqtt_conectado;

void* thread_mqtt_loop(void* arg);
void mqtt_send_command(const char* maquina_id, const char* comando);

#endif

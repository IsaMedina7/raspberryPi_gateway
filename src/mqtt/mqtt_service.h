#ifndef MQTT_SERVICE_H
#define MQTT_SERVICE_H

#include <pthread.h>

// LÍMITE MÁXIMO DE MÁQUINAS (Ajustar según necesidad)
#define MAX_MAQUINAS 5

// Estructura que define UNA sola máquina
typedef struct {
    int id;             // Ej: 1, 2, 3
    char estado[32];    // "IDLE", "WORKING", "ERROR"
    float pos_x;
    float pos_y;
    float pos_z;
    int activa;         // 1 si hemos recibido datos de ella, 0 si no
} MaquinaData;

// Estructura de todo el sistema (Memoria Compartida)
typedef struct {
    // En lugar de variables sueltas, usamos un ARRAY
    MaquinaData maquinas[MAX_MAQUINAS];

    int hay_actualizacion; // Bandera general
    int ultima_maquina_actualizada_id; // Para saber cuál cambió (opcional)
} SystemState;

// Hilo principal del MQTT
void* thread_mqtt_loop(void* arg);

// Función para enviar órdenes
void mqtt_send_command(const char* maquina_id, const char* comando);

#endif


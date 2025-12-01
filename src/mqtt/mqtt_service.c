#include "mqtt_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTAsync.h"
#include "../ui/ui_logic.h" // Necesario para hablar con la pantalla

// Configuración del Broker
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "RPi3_CNC_Central"
#define TOPIC_SUB   "cnc/+/+" // Escuchamos todo
#define QOS         1

#define MQTT_USER   "cnc_admin"
#define MQTT_PASS   "admin1234"

// Variables Globales
MQTTAsync client;
extern SystemState global_state;
extern pthread_mutex_t state_mutex;

// Variable de estado de conexión para la UI
int mqtt_conectado = 0;

// --- HELPERS ---
int extraer_id_maquina(char* topicName) {
    int id = -1;
    sscanf(topicName, "cnc/maquina_%d", &id);
    return id;
}

// --- CALLBACKS DE CONEXIÓN ---

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("[MQTT ERROR] Fallo conexión. Código: %d\n", response ? response->code : 0);
    mqtt_conectado = 0;
}

// ESTA ERA LA DUPLICADA: Solo dejamos una versión
void onConnectionLost(void *context, char *cause) {
    printf("[MQTT WARN] Conexión perdida. Causa: %s\n", cause ? cause : "Desconocida");
    mqtt_conectado = 0;
}

// ESTA TAMBIÉN ESTABA DUPLICADA
void onConnect(void* context, MQTTAsync_successData* response) {
    printf("[MQTT] ¡Conectado al Broker!\n");
    mqtt_conectado = 1;

    // Suscribirse de nuevo al conectar
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_subscribe(client, TOPIC_SUB, QOS, &opts);
}

// --- CALLBACK DE MENSAJES (RECEPCIÓN) ---
int onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    char payload[256];
    int len = message->payloadlen > 255 ? 255 : message->payloadlen;
    strncpy(payload, (char*)message->payload, len);
    payload[len] = '\0';

    // 1. Identificar Máquina
    int id = extraer_id_maquina(topicName);
    if (id < 1 || id > MAX_MAQUINAS) {
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topicName);
        return 1;
    }
    int index = id - 1;

    // 2. Guardar en Memoria Global
    pthread_mutex_lock(&state_mutex);

    global_state.maquinas[index].id = id;
    global_state.maquinas[index].activa = 1;

    if (strstr(topicName, "estado")) {
        // Es un cambio de estado
        snprintf(global_state.maquinas[index].estado, 32, "%s", payload);

        // Log visual en pantalla
        char log_visual[128];
        snprintf(log_visual, sizeof(log_visual), "<< M%d: %s", id, payload);
        ui_add_log(log_visual);

    } else if (strstr(topicName, "posicion")) {
        // Es una actualización de coordenadas (POS:X:Y:Z)
        float x, y, z;
        if (sscanf(payload, "POS:%f:%f:%f", &x, &y, &z) == 3) {
            global_state.maquinas[index].pos_x = x;
            global_state.maquinas[index].pos_y = y;
            global_state.maquinas[index].pos_z = z;
        }
    }

    global_state.hay_actualizacion = 1; // Avisar a UI que repinte
    global_state.ultima_maquina_actualizada_id = id;

    pthread_mutex_unlock(&state_mutex);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

// --- FUNCIÓN DE ENVÍO (PUBLICACIÓN) ---
void mqtt_send_command(const char* maquina_id, const char* comando) {
    if (!client || !mqtt_conectado) {
        printf("[MQTT WARN] Intento de envío sin conexión\n");
        return;
    }

    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    char topic[128];

    snprintf(topic, sizeof(topic), "cnc/%s/comando", maquina_id);

    opts.context = NULL;
    pubmsg.payload = (void*)comando;
    pubmsg.payloadlen = strlen(comando);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    int rc;
    if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
        printf("[MQTT ERROR] Fallo envío a %s\n", topic);
    } else {
        printf("[MQTT TX] %s -> %s\n", comando, topic);
    }
}

// --- HILO PRINCIPAL MQTT ---
void* thread_mqtt_loop(void* arg) {
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    // Crear cliente
    MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, NULL, onConnectionLost, onMessageArrived, NULL);

    // Configurar conexión
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.automaticReconnect = 1;
    conn_opts.username = MQTT_USER;
    conn_opts.password = MQTT_PASS;

    printf("[MQTT] Conectando...\n");
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("[MQTT] Fallo inicio conexión, código %d\n", rc);
    }

    // Bucle infinito para mantener vivo el hilo
    while(1) {
        sleep(1);
    }

    MQTTAsync_destroy(&client);
    return NULL;
}

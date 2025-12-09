#include "mqtt_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTAsync.h"
#include "../ui/ui_logic.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "RPi3_CNC_Central"
#define TOPIC_SUB   "cnc/+/+"
#define QOS         1
#define MQTT_USER   "cnc_admin"
#define MQTT_PASS   "admin1234"

MQTTAsync client;
extern SystemState global_state;
extern pthread_mutex_t state_mutex;
int mqtt_conectado = 0;

int extraer_id_maquina(char* topicName) {
    int id = -1;
    sscanf(topicName, "cnc/maquina_%d", &id);
    return id;
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("[MQTT] Fallo conexión\n");
    mqtt_conectado = 0;
}

void onConnectionLost(void *context, char *cause) {
    printf("[MQTT] Conexión perdida\n");
    mqtt_conectado = 0;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("[MQTT] Conectado\n");
    mqtt_conectado = 1;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_subscribe(client, TOPIC_SUB, QOS, &opts);
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    char payload[256];
    int len = message->payloadlen > 255 ? 255 : message->payloadlen;
    strncpy(payload, (char*)message->payload, len);
    payload[len] = '\0';

    int id = extraer_id_maquina(topicName);
    if (id < 1 || id > MAX_MAQUINAS) {
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topicName);
        return 1;
    }
    int index = id - 1;

    pthread_mutex_lock(&state_mutex);

    // Si es nueva, activar bandera para recargar lista
    if (global_state.maquinas[index].activa == 0) {
        global_state.maquinas[index].activa = 1;
        global_state.lista_cambio = 1;
    }
    global_state.maquinas[index].id = id;

    // --- CLASIFICACIÓN DE MENSAJES ---

    if (strstr(topicName, "estado")) {
        snprintf(global_state.maquinas[index].estado, 32, "%s", payload);
    }
    else if (strstr(topicName, "posicion")) {
        float x,y,z;
        if (sscanf(payload, "POS:%f:%f:%f", &x, &y, &z) == 3) {
            global_state.maquinas[index].pos_x = x;
            global_state.maquinas[index].pos_y = y;
            global_state.maquinas[index].pos_z = z;
        }
    }
    // NUEVO: CAPTURAR IP
    else if (strstr(topicName, "ip")) {
        snprintf(global_state.maquinas[index].ip, 32, "%s", payload);
        // printf("[MQTT] IP M%d: %s\n", id, payload);
    }

    global_state.hay_actualizacion = 1;
    global_state.ultima_maquina_actualizada_id = id;

    pthread_mutex_unlock(&state_mutex);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void mqtt_send_command(const char* maquina_id, const char* comando) {
    if (!client || !mqtt_conectado) return;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    char topic[128];
    snprintf(topic, sizeof(topic), "cnc/%s/comando", maquina_id);
    opts.context = NULL;
    pubmsg.payload = (void*)comando;
    pubmsg.payloadlen = strlen(comando);
    pubmsg.qos = 1;
    pubmsg.retained = 0;
    MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

void* thread_mqtt_loop(void* arg) {
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, NULL, onConnectionLost, onMessageArrived, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.automaticReconnect = 1;
    conn_opts.username = MQTT_USER;
    conn_opts.password = MQTT_PASS;
    MQTTAsync_connect(client, &conn_opts);
    while(1) { sleep(1); }
    MQTTAsync_destroy(&client);
    return NULL;
}

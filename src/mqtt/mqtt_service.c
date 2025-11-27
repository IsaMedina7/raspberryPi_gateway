#include "mqtt_service.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTAsync.h"

#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "RPi3_CNC_Central"
#define TOPIC_SUB   "cnc/+/+" // Suscribirse a cnc/{CUALQUIERA}/{CUALQUIERA}
#define QOS         1

#define MQTT_USER   "cnc_admin"
#define MQTT_PASS   "admin1234"

MQTTAsync client;
extern SystemState global_state;
extern pthread_mutex_t state_mutex;

// --- HELPERS ---

// Función auxiliar para extraer el ID del tópico
// Tópico esperado: "cnc/maquina_{ID}/..."
int extraer_id_maquina(char* topicName) {
    int id = -1;
    // Buscamos el patrón "cnc/maquina_%d"
    // sscanf es muy útil para esto
    sscanf(topicName, "cnc/maquina_%d", &id);
    return id;
}

// --- CALLBACKS ---

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("[MQTT ERROR] Fallo conexión. Código: %d\n", response ? response->code : 0);
}

void onConnectionLost(void *context, char *cause) {
    printf("[MQTT WARN] Conexión perdida. Causa: %s\n", cause ? cause : "Desconocida");
}

int onMessageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    char payload[256];
    int len = message->payloadlen > 255 ? 255 : message->payloadlen;
    strncpy(payload, (char*)message->payload, len);
    payload[len] = '\0';

    // 1. Identificar qué máquina envió el mensaje
    int id = extraer_id_maquina(topicName);

    // Validar que el ID sea válido y esté dentro de nuestro rango
    if (id < 1 || id > MAX_MAQUINAS) {
        printf("[MQTT WARN] ID de máquina inválido o fuera de rango: %d (Tópico: %s)\n", id, topicName);
        MQTTAsync_freeMessage(&message);
        MQTTAsync_free(topicName);
        return 1;
    }

    int index = id - 1; // Array empieza en 0, ID empieza en 1

    // 2. Guardar en la estructura correcta
    pthread_mutex_lock(&state_mutex);

    global_state.maquinas[index].id = id;
    global_state.maquinas[index].activa = 1; // Marcar como vista

    // Analizar qué tipo de dato llegó (estado o posicion)
    if (strstr(topicName, "estado")) {
        snprintf(global_state.maquinas[index].estado, 32, "%s", payload);
        printf("[MQTT] Máquina %d cambió estado a: %s\n", id, payload);
    }
    else if (strstr(topicName, "posicion")) {
        // Aquí parsearíamos el JSON {"x":10...}
        // Por ahora simulamos lectura simple o lo guardamos como string
        // Ejemplo simple: sscanf(payload, "%f,%f", &x, &y);
    }

    global_state.hay_actualizacion = 1;
    global_state.ultima_maquina_actualizada_id = id;

    pthread_mutex_unlock(&state_mutex);

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("[MQTT] Conectado. Suscribiendo a todos los nodos...\n");
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    // Suscribimos a comodín para escuchar a CUALQUIER máquina
    MQTTAsync_subscribe(client, "cnc/+/estado", QOS, &opts);
}

void mqtt_send_command(const char* maquina_id, const char* comando) {
    if (!client) return;
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
        printf("[MQTT TX] Enviando '%s' a %s\n", comando, topic);
    }
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

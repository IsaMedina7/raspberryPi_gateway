#ifndef WEBSOCKET_CMD_H
#define WEBSOCKET_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Lista de palabras clave para terminar la lectura.
 * Se declara como extern para que otros módulos puedan consultarla si es necesario,
 * pero la definición real reside en el archivo .c
 */
extern const char *TERMINATION_KEYWORDS[];

/**
 * @brief Función auxiliar para verificar si una línea COMIENZA con una palabra clave 
 * de terminación, seguida de un espacio, separador, o fin de línea.
 * * @param line La línea recibida (terminada en \n, \r\n o \0).
 * @return 1 si se encuentra la palabra clave al inicio, 0 si no.
 */
int is_termination_keyword(const char *line);

/**
 * @brief Ejecuta un comando a través de websocat contra una IP específica.
 * * Crea un proceso hijo que ejecuta 'websocat' y filtra la salida (grep).
 * Envía el comando especificado y escucha la respuesta hasta encontrar
 * una palabra clave de terminación (definida en TERMINATION_KEYWORDS).
 * * @param ip La dirección IP (y puerto si aplica) del servidor WebSocket.
 * @param command El comando de texto a enviar a través del socket.
 * @return 0 si el comando se envió y se recibió una respuesta de terminación válida.
 * 1 si hubo un error de sistema, de conexión o timeout.
 */
int run_websocket_cmd(const char *ip, const char *command);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_CMD_H
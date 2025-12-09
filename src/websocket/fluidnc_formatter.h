#ifndef FLUIDNC_COMMANDS_H
#define FLUIDNC_COMMANDS_H

#ifdef __cplusplus
extern "C" {
#endif

// Tamaño recomendado para el buffer de comandos
#define FLUIDNC_CMD_MAX 128

/**
 * @brief Formatea comandos de movimiento manual (jogging).
 * @param axis El eje a mover ('X', 'Y', 'Z').
 * @param distance La distancia a mover (positivo o negativo).
 * @param feed_rate Velocidad de avance (mm/min).
 * @param cmd Buffer de salida donde se escribirá el comando.
 */
void fluidnc_format_jog(char axis, float distance, int feed_rate, char *cmd);

/**
 * @brief Genera comando para home (referencia de origen).
 * @param cmd Buffer de salida.
 */
void fluidnc_format_home(char *cmd);

/**
 * @brief Genera comando de parada de emergencia (Feed Hold / Stop).
 * @param cmd Buffer de salida.
 */
void fluidnc_format_stop(char *cmd);

/**
 * @brief Solicita estado actual de la máquina (Status report).
 * @param cmd Buffer de salida.
 */
void fluidnc_format_status(char *cmd);

/**
 * @brief Parsea la respuesta de FluidNC para extraer posición XYZ (MPos).
 * Soporta formato legado (Grbl) y busca patrones JSON básicos.
 * * @param response La cadena recibida desde el WebSocket.
 * @param x Puntero para guardar la posición X.
 * @param y Puntero para guardar la posición Y.
 * @param z Puntero para guardar la posición Z.
 * @return 1 si se encontró y parseó la posición, 0 si no.
 */
int fluidnc_parse_mpos(const char *response, float *x, float *y, float *z);

#ifdef __cplusplus
}
#endif

#endif // FLUIDNC_COMMANDS_H
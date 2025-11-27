#ifndef LOGGER_H
#define LOGGER_H

// Inicializa el sistema de logs (abre el archivo)
void logger_init(void);

// Registra un evento
// Tipo: "INFO", "ERROR", "ALERTA"
// Mensaje: "Maquina 1 inicio corte", "Fallo conexion WiFi"
void logger_log(const char* tipo, const char* mensaje);

#endif


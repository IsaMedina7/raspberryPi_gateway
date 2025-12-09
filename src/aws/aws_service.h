#ifndef AWS_SERVICE_H
#define AWS_SERVICE_H

// --- CONFIGURACIÓN DE LA NUBE ---
// URL de tu API en AWS para reportar el estado
// Asegúrate de cambiar la IP por la IP pública de tu servidor EC2
#define AWS_ENDPOINT_STATUS "http://54.123.45.67:3000/api/maquina/estado"

// Intervalo de reporte de estado en segundos (ej: cada 5 segundos)
#define AWS_STATUS_INTERVAL 5

// --- FUNCIONES PÚBLICAS ---
void* thread_aws_loop(void* arg);
void aws_trigger_update(void); // Forzar actualización inmediata

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h> 
#include "websocket_cmd.h"

// --------------------------------------------------------------------------
// Lista de palabras clave para terminar la lectura. 
const char *TERMINATION_KEYWORDS[] = {
    "ok",
    "error",
    "ready",
    NULL // El array siempre debe terminar con NULL
};

/**
 * @brief Función auxiliar para verificar si una línea COMIENZA con una palabra clave 
 * de terminación, seguida de un espacio, separador, o fin de línea.
 * @param line La línea recibida (terminada en \n, \r\n o \0).
 * @return 1 si se encuentra la palabra clave al inicio, 0 si no.
 */
int is_termination_keyword(const char *line) {
    // Si la línea es NULL o vacía, no hay keyword.
    if (!line || line[0] == '\0') {
        return 0;
    }

    // Recorrer la lista de palabras clave
    for (int i = 0; TERMINATION_KEYWORDS[i] != NULL; ++i) {
        const char *keyword = TERMINATION_KEYWORDS[i];
        size_t keyword_len = strlen(keyword);
        
        // 1. Verificar si la línea empieza con la palabra clave
        if (strncmp(line, keyword, keyword_len) == 0) {
            
            // 2. Verificar el carácter inmediatamente después de la palabra clave.
            char next_char = line[keyword_len];
            
            // Si el siguiente carácter es el fin de la línea, es un match perfecto.
            // Esto cubre: "ok\n" o "error\n"
            if (next_char == '\n' || next_char == '\0') {
                // fprintf(stderr, "DEBUG: Termination keyword '%s' (exact match) received.\n", keyword); //Debug
                return 1;
            }
            
            // Si el siguiente carácter es un espacio, paréntesis, dos puntos, o coma, 
            // asumimos que es un separador válido para metadatos.
            // Esto cubre: "ok (timestamp...", "error: details...", "ready, status..."
            if (next_char == ' ' || next_char == '(' || next_char == ':' || next_char == ',') {
                // fprintf(stderr, "DEBUG: Termination keyword '%s' (prefix match) received.\n", keyword); //Debug
                return 1;
            }
            
            // Manejar caso especial de \r\n
            if (next_char == '\r' && line[keyword_len + 1] == '\n') {
                // fprintf(stderr, "DEBUG: Termination keyword '%s' (prefix match) received.\n", keyword); //Debug
                return 1;
            }
        }
    }
    return 0;
}


// --------------------------------------------------------------------------
// La función run_websocket_cmd (Mantiene la estructura de I/O)
// --------------------------------------------------------------------------
int run_websocket_cmd(const char *ip, const char *command) {
    // ... (Secciones 1 a 7: Inicialización, fork, socketpair, envío del comando, son iguales) ...

    char cmd_buffer[1024];
    if (snprintf(cmd_buffer, sizeof(cmd_buffer), 
                 "websocat ws://%s | grep --line-buffered -v \"^PING\"", ip) >= sizeof(cmd_buffer)) {
        fprintf(stderr, "ERROR: IP o comando del shell demasiado largos.\n");
        return 1;
    }
    const char *cmd = cmd_buffer;

    int sockfds[2]; 
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfds) == -1) {
        perror("socketpair");
        return 1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(sockfds[0]);
        close(sockfds[1]);
        return 1;
    }

    if (pid == 0) {
        close(sockfds[0]);
        if (dup2(sockfds[1], STDIN_FILENO) == -1 || dup2(sockfds[1], STDOUT_FILENO) == -1) {
            _exit(1);
        }
        dup2(STDOUT_FILENO, STDERR_FILENO);
        close(sockfds[1]);
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        perror("execl");
        _exit(127);
    }
    
    close(sockfds[1]); 
    FILE *ws_io = fdopen(sockfds[0], "r+");
    if (!ws_io) {
        perror("fdopen");
        close(sockfds[0]);
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        return 1;
    }
    signal(SIGPIPE, SIG_IGN);

    // Enviar el comando
    fprintf(stderr, "DEBUG: sending '%s' to ws://%s\n", command, ip); //Debug
    fprintf(ws_io, "%s\n", command); //Debug
    fflush(ws_io);
    // --------------------------------------------------------------------------

    // 8. Leer respuestas del hijo (websocat)
    char buf[512];
    int read_success = 0;
    int termination_flag = 0; 

    while (fgets(buf, sizeof(buf), ws_io)) {
        read_success = 1;
        
        // ** NUEVA LÓGICA DE CIERRE FLEXIBLE **
        if (is_termination_keyword(buf)) {
            // Imprimir la línea final
            printf("recv from %s: %s", ip, buf);
            termination_flag = 1;
            break; // Salir del bucle
        }

        // Si no es un keyword de cierre, simplemente se imprime
        printf("recv from %s: %s", ip, buf);
    }
    
    // 9. Cerrar y esperar al hijo
    fclose(ws_io); 
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "WARNING: Child process (websocat) exited with status %d\n", WEXITSTATUS(status));
    }

    return (read_success && termination_flag) ? 0 : 1;
}
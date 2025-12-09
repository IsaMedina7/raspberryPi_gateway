#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

// Simple WebSocket frame parser test
int parse_ws_frame(unsigned char *data, int data_len, char *payload, int max_payload)
{
    if (data_len < 2) return 0;
    
    unsigned char fin = (data[0] & 0x80) >> 7;
    unsigned char opcode = data[0] & 0x0f;
    unsigned char masked = (data[1] & 0x80) >> 7;
    unsigned char payload_len_code = data[1] & 0x7f;
    
    int header_len = 2;
    int frame_len = 0;
    
    if (payload_len_code == 126) {
        if (data_len < 4) return 0;
        frame_len = (data[2] << 8) | data[3];
        header_len = 4;
    } else if (payload_len_code == 127) {
        if (data_len < 10) return 0;
        frame_len = (data[8] << 8) | data[9];
        header_len = 10;
    } else {
        frame_len = payload_len_code;
    }
    
    if (masked) {
        if (data_len < header_len + 4) return 0;
        header_len += 4;
    }
    
    if (data_len < header_len + frame_len) return 0;
    if (frame_len < 0 || frame_len > max_payload) return -1;
    
    unsigned char *payload_start = &data[header_len];
    if (masked) {
        unsigned char *mask = &data[header_len - 4];
        for (int i = 0; i < frame_len; i++) {
            payload[i] = payload_start[i] ^ mask[i % 4];
        }
    } else {
        memcpy(payload, payload_start, frame_len);
    }
    
    payload[frame_len] = '\0';
    return frame_len;
}

int main() {
    printf("=== WebSocket Frame Parser Test ===\n\n");
    
    printf("Frame analysis:\n");
    printf("  89 00 = FIN=1, opcode=0x9 (PING), len=0 (no payload)\n");
    printf("  81 0c = FIN=1, opcode=0x1 (TEXT), len=12 bytes\n");
    printf("  Raw data starts at byte offset 2\n\n");
    
    // Test parsing the actual frame data
    unsigned char real_data[] = {
        0x81, 0x0c,  // TEXT frame, 12 bytes
        'C', 'U', 'R', 'R', 'E', 'N', 'T', '_', 'I', 'D', ':', '0'
    };
    char payload[256];
    int len = parse_ws_frame(real_data, sizeof(real_data), payload, sizeof(payload));
    printf("Parsed real FluidNC frame: '%s' (len=%d)\n\n", payload, len);
    
    // Now test actual connection
    printf("Connecting to FluidNC at 192.168.137.119:81\n");
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("ERROR: Socket creation failed\n");
        return 1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(81);
    
    if (inet_pton(AF_INET, "192.168.137.119", &addr.sin_addr) <= 0) {
        printf("ERROR: Invalid IP\n");
        return 1;
    }
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("ERROR: Connection failed: %s\n", strerror(errno));
        return 1;
    }
    
    printf("Connected!\n");
    
    // Handshake
    char request[512];
    snprintf(request, sizeof(request),
        "GET / HTTP/1.1\r\n"
        "Host: 192.168.137.119:81\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n");
    
    printf("Sending handshake...\n");
    if (send(sock, request, strlen(request), 0) < 0) {
        printf("ERROR: Send failed\n");
        return 1;
    }
    
    char response[1024];
    int n = recv(sock, response, sizeof(response) - 1, 0);
    if (n <= 0) {
        printf("ERROR: No response\n");
        return 1;
    }
    
    response[n] = '\0';
    if (strstr(response, "101") == NULL) {
        printf("ERROR: Handshake failed\n");
        printf("Response:\n%s\n", response);
        return 1;
    }
    
    printf("Handshake successful!\n\n");
    
    // Set non-blocking
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    
    // Send "?" to get status
    printf("Sending status request: '?'\n");
    unsigned char status_frame[] = {
        0x81,  // FIN=1, opcode=1
        0x01,  // Payload len=1, no mask
        '?'
    };
    
    if (send(sock, status_frame, sizeof(status_frame), 0) < 0) {
        printf("ERROR: Send failed\n");
        return 1;
    }
    
    printf("Waiting for response...\n");
    
    // Try to receive multiple frames
    unsigned char rx_buffer[4096];
    int rx_len = 0;
    
    for (int i = 0; i < 20; i++) {
        n = recv(sock, &rx_buffer[rx_len], sizeof(rx_buffer) - rx_len, 0);
        
        if (n > 0) {
            rx_len += n;
            printf("[RX] Received %d bytes (total %d)\n", n, rx_len);
            
            // Parse frames at current position
            int offset = 0;
            while (offset < rx_len) {
                len = parse_ws_frame(&rx_buffer[offset], rx_len - offset, payload, sizeof(payload));
                if (len > 0) {
                    printf("  Frame at offset %d: '%s' (opcode=%d)\n", offset, payload, rx_buffer[offset] & 0x0f);
                    // Skip past this frame
                    // Simple: just skip 2 bytes header + payload
                    offset += 2 + len;  // Rough estimate
                    break;
                } else if (len < 0) {
                    printf("  ERROR: Invalid frame at offset %d\n", offset);
                    break;
                } else {
                    // Incomplete frame, wait for more data
                    printf("  Frame incomplete, waiting for more data\n");
                    break;
                }
            }
        } else if (n == 0) {
            printf("Connection closed\n");
            break;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("ERROR: recv failed: %s\n", strerror(errno));
                break;
            }
            usleep(100000);  // 100ms
        }
    }
    
    close(sock);
    printf("\nTest complete\n");
    return 0;
}

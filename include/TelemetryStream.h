// ============ TelemetryStream.h ============
// Protocolo de Telemetria via TCP
// Monitorização contínua de rovers: posição, bateria, estado

#ifndef TELEMETRYSTREAM_H
#define TELEMETRYSTREAM_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <time.h>

// ============ CONSTANTES ============
#define TELEMETRY_PORT 5006
#define MAX_TELEMETRY_CONNECTIONS 10
#define TELEMETRY_SEND_INTERVAL 1  // Enviar telemetria a cada 5 segundos

// ============ TIPOS DE DADOS ============
typedef enum {
    STATE_IDLE = 0,          // Parado à base
    STATE_IN_MISSION = 1,    // Executando missão
    STATE_RETURNING = 2,     // Retornando à base
    STATE_ERROR = 3,         // Erro
    STATE_CHARGING = 4       // Recarregando
} RoverOperationalState;

// ============ ESTRUTURA: MENSAGEM DE TELEMETRIA ============
#pragma pack(push, 1)
typedef struct {
    uint32_t timestamp;              // Unix timestamp
    
    char rover_id[32];               // ID do rover
    float position_x;                // Coordenada X
    float position_y;                // Coordenada Y
    uint8_t battery;                 // % Bateria (0-100)
    uint8_t state;                   // Estado operacional (RoverOperationalState)
    float temperature;               // Temperatura interna (°C)
    uint8_t signal_strength;         // Força do sinal (0-100)
    
    uint32_t nonce;                  // Número aleatório (segurança)
} TelemetryMessage;
#pragma pack(pop)

// ============ ESTRUTURA: SESSÃO DE TELEMETRIA ============
typedef struct {
    char rover_id[32];               // ID do rover
    float last_position_x;           // Última posição conhecida
    float last_position_y;
    uint8_t last_battery;            // Última bateria conhecida
    uint8_t last_state;              // Último estado operacional
    float last_temperature;          // Última temperatura conhecida
    uint8_t last_signal_strength;    // Último sinal conhecido
    time_t last_update;              // Timestamp do último update
    
    int sockfd;                      // Socket TCP do rover
    struct sockaddr_in addr;         // Endereço do rover
    int active;                      // Flag: ativo ou inativo
} TelemetrySession;

// ============ FUNÇÕES: SERVIDOR (NAVE-MÃE) ============

// Criar socket servidor TCP para telemetria
int create_telemetry_server(int port);

// Aceitar nova conexão TCP de rover
void accept_telemetry_connection(int server_fd, TelemetrySession *sessions, int *count);

// Receber dados de telemetria de um rover
void receive_telemetry_data(TelemetrySession *session);

// Armazenar dados de telemetria no histórico
void store_telemetry(TelemetrySession *session, TelemetryMessage *msg);

// Imprimir status de telemetria
void print_telemetry_status(TelemetrySession *sessions, int count);

// ============ FUNÇÕES: CLIENTE (ROVER) ============

// Conectar ao servidor de telemetria
int create_telemetry_connection(const char *host, int port);

// Enviar mensagem de telemetria
void send_telemetry_message(int fd, TelemetryMessage *msg);

#endif // TELEMETRYSTREAM_H
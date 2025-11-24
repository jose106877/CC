// ============ MissionLink.h ============
// Protocolo de Comunicação MissionLink (ML)
// Camada de Transporte: UDP (porta 5005)
// Funcionalidade: Atribuição e execução de missões entre Nave-Mãe e Rovers
//
// ESPECIFICAÇÃO DO PROTOCOLO:
// ============================
// 1. HANDSHAKE: Byte 0xFF para iniciar conexão
// 2. Todos os pacotes têm 228 bytes (sizeof(Packet))
// 3. Sequência de mensagens:
//    - Rover: REQUEST (seq=N)
//    - Servidor: ASSIGN (seq=N+1) com parâmetros de missão
//    - Rover: PROGRESS × N (seq incremental)
//    - Rover: COMPLETE (seq final)
//    - Ambos: ACK confirmando recepção
// 4. Confiabilidade: ACK obrigatório, retransmissão automática (5 tentativas)

#ifndef MISSIONLINK_H
#define MISSIONLINK_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <unistd.h>

// ============ CONSTANTES ============
#define PORT 5005
#define MAX_ROVERS 5
#define MAX_MISSIONS 100
#define HANDSHAKE_TIMEOUT 2
#define HANDSHAKE_RETRIES 5
#define ACK_TIMEOUT 1
#define ACK_RETRIES 5
#define ROVER_ACTIVITY_TIMEOUT 5
#define FRAG_SIZE 1024

// ============ TIPOS DE PACOTES ============
typedef enum {
    PKT_MISSION_REQUEST = 1,   // Rover → Servidor: Solicita missão
    PKT_MISSION_ASSIGN = 2,    // Servidor → Rover: Atribui missão com parâmetros
    PKT_PROGRESS = 3,          // Rover → Servidor: Reporta progresso
    PKT_COMPLETE = 4,          // Rover → Servidor: Missão concluída
    PKT_ACK = 5                // Ambos: Confirmação de recepção
} PacketType;

// ============ ESTRUTURA DO PACOTE ============
#pragma pack(push, 1)
typedef struct {
    // CABEÇALHO (12 bytes)
    uint8_t type;               // PacketType enum
    uint32_t seq;               // Número de sequência
    uint8_t battery;            // % Bateria (0-100)
    uint8_t progress;           // % Progresso (0-100)
    uint32_t nonce;             // Número aleatório para segurança

    // IDENTIFICADORES (96 bytes)
    char rover_id[32];          // ID do rover (ex: "R-001")
    char mission_id[32];        // ID da missão (ex: "M-001")
    char task_type[64];         // Tipo de tarefa (ex: "scan_area")

    // PARÂMETROS DE MISSÃO (16 bytes)
    // Enviados apenas em PKT_MISSION_ASSIGN
    float mission_x1, mission_y1;  // Coordenada inicial (canto inferior-esquerdo)
    float mission_x2, mission_y2;  // Coordenada final (canto superior-direito)

    // DEFINIÇÕES DE MISSÃO (8 bytes)
    uint32_t mission_duration;     // Duração máxima em segundos
    uint32_t update_interval;      // Intervalo entre atualizações em segundos

} Packet;
#pragma pack(pop)


// ============ FUNÇÕES UDP BÁSICAS ============
int create_udp_socket(void);
void bind_udp_socket(int sockfd, int port, struct sockaddr_in *addr);
int receive_udp(int sockfd, char *buffer, size_t buf_size, 
                struct sockaddr_in *client_addr);
ssize_t send_udp_with_ack(int sockfd, struct sockaddr_in *server_addr, 
                          char *data, size_t size);
void send_ack_packet(int sockfd, struct sockaddr_in *addr, uint32_t seq);

// ============ FUNÇÕES UTILITÁRIAS ============
void print_timestamp(void);
const char* get_packet_type_name(uint8_t type);
void print_packet_info(Packet *pkt);

#endif // MISSIONLINK_H
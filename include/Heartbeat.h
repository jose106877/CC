// ============ Heartbeat.h (ATUALIZADO) ============
// Sistema de heartbeat para verificar saúde dos rovers
// A Nave-Mãe envia PING periodicamente, rover responde com PONG
#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "MissionLink.h"
#include "Server_management.h"

// ============ TIPOS DE HEARTBEAT ============
typedef enum {
    PKT_PING = 10,    // Nave-Mãe → Rover: "Estás aí?"
    PKT_PONG = 11     // Rover → Nave-Mãe: "Sim, estou aqui!"
} HeartbeatPacketType;

// ============ CONSTANTES ============
#define HEARTBEAT_INTERVAL 30        // Enviar PING a cada 30 segundos (ALTERADO: era 60)
#define HEARTBEAT_TIMEOUT 5          // Timeout para resposta: 5 segundos (ALTERADO: era 10)
#define HEARTBEAT_MAX_RETRIES 2      // Máximo de PINGs sem resposta antes de marcar inativo (ALTERADO: era 1)

// ============ ESTRUTURA: ESTADO DE HEARTBEAT ============
typedef struct {
    time_t last_ping_sent;           // Hora do último PING enviado
    time_t last_pong_received;       // Hora do último PONG recebido
    int consecutive_missed_pongs;    // PINGs sem resposta consecutivos
    int is_healthy;                  // 1=saudável, 0=inativo/morto
} HeartbeatState;

// ============ FUNÇÕES SERVIDOR (NAVE-MÃE) ============

// Verificar e enviar PING para rovers inativos
void check_and_send_heartbeats(int sockfd, RoverSession *sessions, int num_rovers);

// Processar PONG recebido
void process_pong(RoverSession *rover, HeartbeatState *heartbeat);

// Marcar rover como inativo
void mark_rover_inactive(RoverSession *rover, HeartbeatState *heartbeat);

// ============ FUNÇÕES CLIENTE (ROVER) ============

// Processar PING recebido e enviar PONG
void process_ping_and_respond(int sockfd, Packet *ping_pkt, struct sockaddr_in *server_addr, const char *rover_id);

// Imprimir status de heartbeat
void print_heartbeat_status(RoverSession *sessions, int num_rovers);

#endif // HEARTBEAT_H
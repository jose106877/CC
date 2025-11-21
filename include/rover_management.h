// ============ Client_management.h ============
// Gestão de estado do cliente (Rover)
#ifndef ROVER_MANAGEMENT_H
#define ROVER_MANAGEMENT_H

#include "MissionLink.h"

// ============ ESTRUTURA: ESTADO DO ROVER ============
typedef struct {
    char rover_id[32];              // ID do rover
    uint32_t seq;                   // Próximo número de sequência
    uint8_t battery;                // % Bateria
    uint8_t progress;               // % Progresso da missão atual
    
    // Missão atual
    char mission_id[32];            // ID da missão
    char task_type[64];             // Tipo de tarefa
    float mission_x1, mission_y1;   // Coordenadas da área
    float mission_x2, mission_y2;
    uint32_t mission_duration;      // Duração máxima
    uint32_t update_interval;       // Intervalo entre updates
    
    int has_mission;                // Flag: tem missão ativa? (1=sim)
} RoverState;

// ============ FUNÇÕES ============

// Inicializar estado do rover
void init_rover_state(RoverState *state, const char *rover_id);

// Armazenar missão recebida
void store_mission_assignment(RoverState *state, Packet *assign_pkt);

// Incrementar sequência
void next_sequence(RoverState *state);

// Preparer pacote REQUEST
void prepare_request_packet(RoverState *state, Packet *pkt);

// Preparar pacote PROGRESS
void prepare_progress_packet(RoverState *state, Packet *pkt, uint8_t progress, uint8_t battery);

// Preparar pacote COMPLETE
void prepare_complete_packet(RoverState *state, Packet *pkt, uint8_t battery);

#endif // ROVER_MANAGEMENT_H
// ============ Client_management.c ============
// Implementação de gestão de estado do cliente (Rover)
#include "rover_management.h"
#include <string.h>
#include <stdlib.h>

// Inicializar estado
void init_rover_state(RoverState *state, const char *rover_id) {
    memset(state, 0, sizeof(*state));
    strncpy(state->rover_id, rover_id, sizeof(state->rover_id) - 1);
    state->seq = 1;
    state->battery = 100;
    state->progress = 0;
    state->has_mission = 0;
}

// Armazenar missão recebida
void store_mission_assignment(RoverState *state, Packet *assign_pkt) {
    strncpy(state->mission_id, assign_pkt->mission_id, sizeof(state->mission_id) - 1);
    strncpy(state->task_type, assign_pkt->task_type, sizeof(state->task_type) - 1);
    state->mission_x1 = assign_pkt->mission_x1;
    state->mission_y1 = assign_pkt->mission_y1;
    state->mission_x2 = assign_pkt->mission_x2;
    state->mission_y2 = assign_pkt->mission_y2;
    state->mission_duration = assign_pkt->mission_duration;
    state->update_interval = assign_pkt->update_interval;
    state->has_mission = 1;
}

// Incrementar sequência
void next_sequence(RoverState *state) {
    state->seq++;
}

// Preparar REQUEST
void prepare_request_packet(RoverState *state, Packet *pkt) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->type = PKT_MISSION_REQUEST;
    pkt->seq = state->seq;
    pkt->battery = state->battery;
    pkt->progress = 0;
    pkt->nonce = rand() % 100000;
    strncpy(pkt->rover_id, state->rover_id, sizeof(pkt->rover_id) - 1);
}

// Preparar PROGRESS
void prepare_progress_packet(RoverState *state, Packet *pkt, uint8_t progress, uint8_t battery) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->type = PKT_PROGRESS;
    pkt->seq = state->seq;
    pkt->battery = battery;
    pkt->progress = progress;
    pkt->nonce = rand() % 100000;
    strncpy(pkt->rover_id, state->rover_id, sizeof(pkt->rover_id) - 1);
    strncpy(pkt->mission_id, state->mission_id, sizeof(pkt->mission_id) - 1);
    strncpy(pkt->task_type, state->task_type, sizeof(pkt->task_type) - 1);
}

// Preparar COMPLETE
void prepare_complete_packet(RoverState *state, Packet *pkt, uint8_t battery) {
    memset(pkt, 0, sizeof(*pkt));
    pkt->type = PKT_COMPLETE;
    pkt->seq = state->seq;
    pkt->battery = battery;
    pkt->progress = 100;
    pkt->nonce = rand() % 100000;
    strncpy(pkt->rover_id, state->rover_id, sizeof(pkt->rover_id) - 1);
    strncpy(pkt->mission_id, state->mission_id, sizeof(pkt->mission_id) - 1);
    strncpy(pkt->task_type, state->task_type, sizeof(pkt->task_type) - 1);
}
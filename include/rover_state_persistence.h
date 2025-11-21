// ============ rover_state_persistence.h ============
// Persistência de estado do rover em arquivo binário
#ifndef ROVER_STATE_PERSISTENCE_H
#define ROVER_STATE_PERSISTENCE_H

#include "rover_management.h"
#include <stdint.h>

// ============ ESTRUTURA: ESTADO PERSISTIDO ============
typedef struct {
    char rover_id[32];              // ID do rover
    char mission_id[32];            // ID da missão atual
    char task_type[64];             // Tipo de tarefa
    uint32_t seq;                   // Número de sequência
    uint8_t battery;                // % Bateria
    uint8_t progress;               // % Progresso
    float position_x;               // Posição X
    float position_y;               // Posição Y
    uint32_t timestamp;             // Timestamp do estado
} RoverPersistedState;

// ============ FUNÇÕES DE PERSISTÊNCIA ============

// Inicializar arquivo de estado (criar se não existir)
void init_state_file(const char *rover_id);

// Guardar estado atual do rover em arquivo binário
void save_rover_state(const char *rover_id, RoverState *state, 
                      float position_x, float position_y);

// Carregar último estado do rover do arquivo binário
int load_rover_state(const char *rover_id, RoverState *state,
                     float *position_x, float *position_y);

// Obter nome do arquivo de estado
void get_state_filename(const char *rover_id, char *filename, size_t size);

// Imprimir estado persistido
void print_persisted_state(RoverPersistedState *state);

#endif // ROVER_STATE_PERSISTENCE_H
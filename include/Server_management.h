// ============ Server_management.h (ATUALIZADO) ============
// Gestão de rovers e missões no servidor (Nave-Mãe)
#ifndef SERVER_MANAGEMENT_H
#define SERVER_MANAGEMENT_H

#include "MissionLink.h"
#include <sys/socket.h>
#include <time.h>

// ============ ESTRUTURA: ROVER NA NAVE-MÃE ============
typedef struct {
    char rover_id[32];              // Identificador único
    uint32_t last_seq;              // Última sequência recebida
    char mission_id[32];            // Missão atual
    char task_type[64];             // Tipo de tarefa atual
    uint8_t battery;                // % Bateria
    uint8_t progress;               // % Progresso
    time_t last_update;             // Timestamp do último pacote
    time_t last_ping_sent;          // Timestamp do último PING enviado
    struct sockaddr_in addr;        // Endereço do rover
    int active;                     // Flag de atividade (1=ativo, 0=inativo)
    
    // ===== NOVO: Rastreamento de PING/PONG =====
    int waiting_for_pong;           // 1 se aguardando PONG, 0 caso contrário
    int consecutive_missed_pongs;   // Contador de PONGs não recebidos
} RoverSession;

// ============ ESTRUTURA: MISSÃO NA NAVE-MÃE ============
typedef struct {
    char mission_id[32];            // Identificador único
    char rover_id[32];              // Rover atribuído
    char task_type[64];             // Tipo de tarefa
    uint8_t progress;               // % Progresso
    uint8_t battery;                // % Bateria do rover
    time_t start_time;              // Hora de criação
    time_t last_update;             // Hora do último update
    int updates_count;              // Número de updates recebidos
    
    // Parâmetros da missão
    float x1, y1, x2, y2;           // Coordenadas da área
    uint32_t duration;              // Duração máxima em segundos
    uint32_t update_interval;       // Intervalo entre updates em segundos
    int completed;                  // Flag de conclusão (1=concluída)
} MissionRecord;

// ============ FUNÇÕES DE GESTÃO ============

// Inicializar tabelas de rovers e missões
void init_server_tables(void);

// Criar nova missão para um rover
MissionRecord* create_mission_for_rover(const char *rover_id);

// Atualizar informações de uma missão
void add_or_update_mission(const char *mission_id, uint8_t progress, uint8_t battery);

// Marcar missão como concluída
void mark_mission_complete(const char *mission_id);

// Obter sessão de rover (sem modificar)
RoverSession* get_rover_session(const char *rover_id);

// Registar ou atualizar sessão de rover
RoverSession* register_or_update_rover(const char *rover_id, struct sockaddr_in *addr);

// Imprimir tabela de missões
void print_mission_status(void);

// Imprimir tabela de rovers
void print_rover_status(void);

#endif // SERVER_MANAGEMENT_H
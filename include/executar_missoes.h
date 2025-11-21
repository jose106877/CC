// ============ executar_missoes.h ============
// Execução de diferentes tipos de missões com lógica individual
#ifndef EXECUTAR_MISSOES_H
#define EXECUTAR_MISSOES_H

#include "rover_management.h"
#include "Server_management.h"
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

// ============ ESTRUTURA AUXILIAR ============
typedef struct {
    int total_steps;      // Total de passos/ações da missão
    int current_step;     // Passo atual
    uint8_t progress;     // Progresso em percentagem
} MissionProgress;

// ============ FUNÇÕES DE EXECUÇÃO DE MISSÕES ============

// Configurar missão CAPTURE_IMAGES (5 fotos, 20% cada)
MissionRecord* capture_images(MissionRecord *mission);

// Configurar missão ANALYZE_SOIL (5 análises, 20% cada)
MissionRecord* analyze_soil(MissionRecord *mission);

// Configurar missão COLLECT_SAMPLES (4 amostras, 25% cada)
MissionRecord* collect_samples(MissionRecord *mission);

// Configurar missão SCAN_AREA (10 setores, 10% cada)
MissionRecord* scan_area(MissionRecord *mission);

// Configurar missão DEPLOY_SENSOR (5 sensores, 20% cada)
MissionRecord* deploy_sensor(MissionRecord *mission);

// ============ FUNÇÕES GENÉRICAS ============

// Executar a missão correta baseada no tipo de tarefa
MissionRecord* execute_mission_logic(MissionRecord *mission, const char *task_type);

// Executar progresso da missão com atualizações periódicas
void execute_mission_progress(int sockfd, struct sockaddr_in *server_addr, 
                              RoverState *state, uint8_t *progress, uint8_t *battery);

// Enviar atualização de progresso
void send_progress_update(int sockfd, struct sockaddr_in *server_addr,
                          RoverState *state, uint8_t progress, uint8_t battery);

#endif // EXECUTAR_MISSOES_H
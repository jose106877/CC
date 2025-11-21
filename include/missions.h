// ============ missions.h ============
// Definições e funções para execução de missões com lógica individual
#ifndef MISSIONS_H
#define MISSIONS_H

#include "Server_management.h"
#include <stdint.h>

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

// Calcular progresso da ação (baseado em passos completados)
uint8_t calculate_step_progress(const char *task_type, int step_completed);

// Obter número total de passos para uma missão
int get_mission_total_steps(const char *task_type);

// Obter descrição legível da ação em progresso
const char* get_step_description(const char *task_type, int current_step, int total_steps);

#endif // MISSIONS_H
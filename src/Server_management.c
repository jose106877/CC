// ============ Server_management.c (ATUALIZADO) ============
// Implementação da gestão de rovers e missões
#include "Server_management.h"
#include "missions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Tabelas globais
RoverSession sessions[MAX_ROVERS];
MissionRecord missions[MAX_MISSIONS];
int num_sessions = 0;
int num_missions = 0;
int next_mission_id = 1;

// Inicializar tabelas
void init_server_tables(void) {
    memset(sessions, 0, sizeof(sessions));
    memset(missions, 0, sizeof(missions));
    num_sessions = 0;
    num_missions = 0;
    next_mission_id = 1;
}

// Criar missão para um rover
MissionRecord* create_mission_for_rover(const char *rover_id) {
    if (num_missions >= MAX_MISSIONS) {
        print_timestamp();
        printf("[ML] ✗ Limite de missões atingido\n");
        return NULL;
    }

    MissionRecord *mission = &missions[num_missions++];
    memset(mission, 0, sizeof(*mission));
    
    snprintf(mission->mission_id, sizeof(mission->mission_id), "M-%03d", next_mission_id++);
    strncpy(mission->rover_id, rover_id, sizeof(mission->rover_id) - 1);
    
    // Gerar tarefa aleatória
    const char *tasks[] = {"analyze_soil", "capture_images", "collect_samples", "scan_area", "deploy_sensor"};
    int task_idx = rand() % 5;
    strncpy(mission->task_type, tasks[task_idx], sizeof(mission->task_type) - 1);

    execute_mission_logic(mission, mission->task_type);

    
    // Gerar duração e intervalo
    //mission->duration = 300 + (rand() % 600);
    mission->update_interval = 10;
    
    mission->start_time = time(NULL);
    mission->last_update = time(NULL);
    //mission->progress = 0;
    mission->battery = 100;
    mission->updates_count = 0;
    mission->completed = 0;
    
    print_timestamp();
    printf("🔋 [ML] MISSÃO CRIADA:\n");
    printf("   ID:            %s\n", mission->mission_id);
    printf("   Para Rover:    %s\n", mission->rover_id);
    printf("   Tarefa:        %s\n", mission->task_type);
    printf("   Área:          (%.1f, %.1f) → (%.1f, %.1f)\n", 
           mission->x1, mission->y1, mission->x2, mission->y2);
    printf("   Duração Máx:   %u segundos\n", mission->duration);
    printf("   Intervalo:     %u segundos\n\n", mission->update_interval);
    
    return mission;
}

// Atualizar missão
void add_or_update_mission(const char *mission_id, uint8_t progress, uint8_t battery) {
    for (int i = 0; i < num_missions; i++) {
        if (strcmp(missions[i].mission_id, mission_id) == 0) {
            missions[i].progress = progress;
            missions[i].battery = battery;
            missions[i].last_update = time(NULL);
            missions[i].updates_count++;
            return;
        }
    }
}

// Marcar como concluída
void mark_mission_complete(const char *mission_id) {
    for (int i = 0; i < num_missions; i++) {
        if (strcmp(missions[i].mission_id, mission_id) == 0) {
            missions[i].completed = 1;
            return;
        }
    }
}

// Obter sessão de rover
RoverSession* get_rover_session(const char *rover_id) {
    for (int i = 0; i < num_sessions; i++) {
        if (strcmp(sessions[i].rover_id, rover_id) == 0) {
            return &sessions[i];
        }
    }
    return NULL;
}

// Registar ou atualizar rover
RoverSession* register_or_update_rover(const char *rover_id, struct sockaddr_in *addr) {
    RoverSession *session = get_rover_session(rover_id);
    
    if (session) {
        session->addr = *addr;
        session->last_update = time(NULL);
        return session;
    }
    
    if (num_sessions < MAX_ROVERS) {
        session = &sessions[num_sessions++];
        memset(session, 0, sizeof(*session));
        strncpy(session->rover_id, rover_id, sizeof(session->rover_id) - 1);
        session->last_seq = 0;
        session->addr = *addr;
        session->active = 1;
        session->last_update = time(NULL);
        session->last_ping_sent = time(NULL);
        
        // ===== INICIALIZAR CAMPOS DE HEARTBEAT =====
        session->waiting_for_pong = 0;
        session->consecutive_missed_pongs = 0;
        
        print_timestamp();
        printf("🆕 Novo Rover conectado: %s\n\n", rover_id);
        
        return session;
    }
    
    return NULL;
}

// Imprimir status de missões
void print_mission_status(void) {
    print_timestamp();
    printf("\n╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║              📊 ESTADO DAS MISSÕES EM CURSO                      ║\n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    printf("║ ID     │ Rover   │ Tarefa           │ Progr │ Bat │ Updates │ Status ║\n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    
    if (num_missions == 0) {
        printf("║ Nenhuma missão ativa                                              ║\n");
    } else {
        for (int i = 0; i < num_missions; i++) {
            const char *status = missions[i].completed ? "✅" : "⏳";
            
            printf("║ %-6s │ %-7s │ %-16s │ %3u%% │ %3u%% │ %7d │ %s      ║\n",
                   missions[i].mission_id,
                   missions[i].rover_id,
                   missions[i].task_type,
                   missions[i].progress,
                   missions[i].battery,
                   missions[i].updates_count,
                   status);
        }
    }
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");
}

// Imprimir status de rovers
void print_rover_status(void) {
    print_timestamp();
    printf("\n╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    🤖 STATUS DOS ROVERS                           ║\n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    printf("║ Rover   │ Status   │ Missão   │ Progr │ Bat │ Seq  │ Último Update  ║\n");
    printf("╠════════════════════════════════════════════════════════════════════╣\n");
    
    if (num_sessions == 0) {
        printf("║ Nenhum rover conectado                                            ║\n");
    } else {
        for (int i = 0; i < num_sessions; i++) {
            if (!sessions[i].active) continue;
            
            time_t time_since_update = time(NULL) - sessions[i].last_update;
            const char *active = time_since_update < 35 ? "✓ ATIVO" : "✗ INATIVO";
            
            printf("║ %-7s │ %-8s │ %-8s │ %3u%% │ %3u%% │ %4u │ %3lds atrás  ║\n",
                   sessions[i].rover_id,
                   active,
                   sessions[i].mission_id[0] ? sessions[i].mission_id : "N/A",
                   sessions[i].progress,
                   sessions[i].battery,
                   sessions[i].last_seq,
                   time_since_update);
        }
    }
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");
}
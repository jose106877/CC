// ============ salvar_estado.c ============
// Implementação de persistência de estado do rover
#include "rover_state_persistence.h"
#include "MissionLink.h"
#include "rover_management.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

// ============ FUNÇÕES DE PERSISTÊNCIA ============

// Obter nome do arquivo de estado
void get_state_filename(const char *rover_id, char *filename, size_t size) {
    snprintf(filename, size, "rovers/rover_%s_state.bin", rover_id);
}

// Inicializar arquivo de estado (criar se não existir)
void init_state_file(const char *rover_id) {
    char filename[64];
    get_state_filename(rover_id, filename, sizeof(filename));
    
    FILE *f = fopen(filename, "rb");
    if (f != NULL) {
        // Arquivo já existe
        fclose(f);
        print_timestamp();
        printf("📁 Arquivo de estado encontrado: %s\n\n", filename);
        return;
    }
    
    // Criar novo arquivo com estado inicial vazio
    RoverPersistedState init_state;
    memset(&init_state, 0, sizeof(init_state));
    strncpy(init_state.rover_id, rover_id, sizeof(init_state.rover_id) - 1);
    init_state.battery = 100;
    init_state.progress = 0;
    init_state.position_x = 0.0;
    init_state.position_y = 0.0;
    init_state.timestamp = (uint32_t)time(NULL);
    
    f = fopen(filename, "wb");
    if (f == NULL) {
        print_timestamp();
        printf("❌ Erro ao criar arquivo de estado: %s\n\n", filename);
        return;
    }
    
    fwrite(&init_state, sizeof(RoverPersistedState), 1, f);
    fclose(f);
    
    print_timestamp();
    printf("📁 Arquivo de estado criado: %s\n\n", filename);
}

// Guardar estado atual do rover em arquivo binário
void save_rover_state(const char *rover_id, RoverState *state,
                      float position_x, float position_y) {
    if (!state) return;
    
    char filename[64];
    get_state_filename(rover_id, filename, sizeof(filename));
    
    RoverPersistedState persisted;
    memset(&persisted, 0, sizeof(persisted));
    
    // Copiar dados do estado
    strncpy(persisted.rover_id, state->rover_id, sizeof(persisted.rover_id) - 1);
    strncpy(persisted.mission_id, state->mission_id, sizeof(persisted.mission_id) - 1);
    strncpy(persisted.task_type, state->task_type, sizeof(persisted.task_type) - 1);
    
    persisted.seq = state->seq;
    persisted.battery = state->battery;
    persisted.progress = state->progress;
    persisted.position_x = position_x;
    persisted.position_y = position_y;
    persisted.timestamp = (uint32_t)time(NULL);
    
    // Escrever no arquivo
    FILE *f = fopen(filename, "wb");
    if (f == NULL) {
        print_timestamp();
        printf("❌ Erro ao guardar estado: não conseguiu abrir %s\n\n", filename);
        return;
    }
    
    size_t written = fwrite(&persisted, sizeof(RoverPersistedState), 1, f);
    fclose(f);
    
    if (written == 1) {
        print_timestamp();
        printf("💾 Estado guardado em %s\n", filename);
        printf("   Bateria: %u%% | Progresso: %u%% | Pos: (%.2f, %.2f)\n",
               persisted.battery, persisted.progress,
               persisted.position_x, persisted.position_y);
        printf("   Sequência: %u | Timestamp: %u\n\n",
               persisted.seq, persisted.timestamp);
    } else {
        print_timestamp();
        printf("❌ Erro ao escrever estado no arquivo\n\n");
    }
}

// Carregar último estado do rover do arquivo binário
int load_rover_state(const char *rover_id, RoverState *state,
                     float *position_x, float *position_y) {
    if (!state) return 0;
    
    char filename[64];
    get_state_filename(rover_id, filename, sizeof(filename));
    
    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        print_timestamp();
        printf("⚠️  Arquivo de estado não encontrado: %s\n", filename);
        printf("   Inicializando com valores padrão\n\n");
        return 0;
    }
    
    RoverPersistedState persisted;
    size_t read = fread(&persisted, sizeof(RoverPersistedState), 1, f);
    fclose(f);
    
    if (read != 1) {
        print_timestamp();
        printf("❌ Erro ao ler arquivo de estado\n\n");
        return 0;
    }
    
    // Verificar se há dados válidos (bateria > 0 ou missão definida)
    if (persisted.battery == 0 && persisted.mission_id[0] == '\0') {
        print_timestamp();
        printf("⚠️  Arquivo de estado vazio\n");
        printf("   Inicializando com valores padrão\n\n");
        return 0;
    }
    
    // Restaurar estado
    strncpy(state->rover_id, persisted.rover_id, sizeof(state->rover_id) - 1);
    strncpy(state->mission_id, persisted.mission_id, sizeof(state->mission_id) - 1);
    strncpy(state->task_type, persisted.task_type, sizeof(state->task_type) - 1);
    
    state->seq = persisted.seq;
    state->battery = persisted.battery;
    state->progress = persisted.progress;
    state->mission_duration = 0;
    state->update_interval = 0;
    state->has_mission = 0;
    
    *position_x = persisted.position_x;
    *position_y = persisted.position_y;
    
    print_timestamp();
    printf("📂 Estado carregado de %s\n", filename);
    printf("   Bateria: %u%% | Progresso: %u%% | Pos: (%.2f, %.2f)\n",
           persisted.battery, persisted.progress,
           persisted.position_x, persisted.position_y);
    printf("   Sequência: %u | Timestamp: %u\n\n",
           persisted.seq, persisted.timestamp);
    
    return 1;
}

// Imprimir estado persistido
void print_persisted_state(RoverPersistedState *state) {
    if (!state) return;
    
    print_timestamp();
    printf("📋 ESTADO PERSISTIDO DO ROVER\n");
    printf("   Rover ID:      %s\n", state->rover_id);
    printf("   Mission ID:    %s\n", state->mission_id);
    printf("   Task Type:     %s\n", state->task_type);
    printf("   Sequência:     %u\n", state->seq);
    printf("   Bateria:       %u%%\n", state->battery);
    printf("   Progresso:     %u%%\n", state->progress);
    printf("   Posição:       (%.2f, %.2f)\n", state->position_x, state->position_y);
    printf("   Timestamp:     %u\n\n", state->timestamp);
}
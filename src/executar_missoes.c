// ============ executar_missoes.c ============
// Execução de diferentes tipos de missões com lógica individual
#include "Server_management.h"
#include "rover_management.h"
#include "Heartbeat.h"
#include "rover_state_persistence.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

// ============ ESTRUTURA AUXILIAR ============
typedef struct
{
    int total_steps;  // Total de passos/ações da missão
    int current_step; // Passo atual
    uint8_t progress; // Progresso em percentagem
} MissionProgress;

// ============ CAPTURE_IMAGES ============
// 5 fotos = 5 passos, cada foto = 20% progresso
MissionRecord *capture_images(MissionRecord *mission)
{
    mission->x1 = 10.0;
    mission->y1 = 10.0;
    mission->x2 = 30.0;
    mission->y2 = 30.0;
    mission->duration = 300;

    // Armazenar info da missão em campos disponíveis
    mission->progress = 0; // Iniciar em 0%

    print_timestamp();
    printf("[MISSÃO] CAPTURE_IMAGES: 5 fotos a tirar\n");
    printf("   Área: (%.1f,%.1f) -> (%.1f,%.1f)\n",
           mission->x1, mission->y1, mission->x2, mission->y2);
    printf("   Progresso: +20%% por cada foto\n");

    return mission;
}

// ============ ANALYZE_SOIL ============
// 5 análises = 5 passos, cada análise = 20% progresso
MissionRecord *analyze_soil(MissionRecord *mission)
{
    mission->x1 = 20.0;
    mission->y1 = 20.0;
    mission->x2 = 25.0;
    mission->y2 = 25.0;
    mission->duration = 600;

    mission->progress = 0;

    print_timestamp();
    printf("[MISSÃO] ANALYZE_SOIL: 5 análises de solo\n");
    printf("   Zona: (%.1f,%.1f)\n", mission->x1, mission->y1);
    printf("   Progresso: +20%% por cada análise\n");

    return mission;
}

// ============ COLLECT_SAMPLES ============
// 4 amostras = 4 passos, cada amostra = 25% progresso
MissionRecord *collect_samples(MissionRecord *mission)
{
    mission->x1 = 5.0;
    mission->y1 = 5.0;
    mission->x2 = 45.0;
    mission->y2 = 45.0;
    mission->duration = 900;

    mission->progress = 0;

    print_timestamp();
    printf("[MISSÃO] COLLECT_SAMPLES: 4 amostras a recolher\n");
    printf("   Área: (%.1f,%.1f) -> (%.1f,%.1f)\n",
           mission->x1, mission->y1, mission->x2, mission->y2);
    printf("   Progresso: +25%% por cada amostra\n");

    return mission;
}

// ============ SCAN_AREA ============
// 10 setores = 10 passos, cada setor = 10% progresso
MissionRecord *scan_area(MissionRecord *mission)
{
    mission->x1 = 0.0;
    mission->y1 = 0.0;
    mission->x2 = 50.0;
    mission->y2 = 50.0;
    mission->duration = 400;

    mission->progress = 0;

    print_timestamp();
    printf("[MISSÃO] SCAN_AREA: 10 setores a varrer\n");
    printf("   Área: (%.1f,%.1f) -> (%.1f,%.1f)\n",
           mission->x1, mission->y1, mission->x2, mission->y2);
    printf("   Progresso: +10%% por cada setor\n");

    return mission;
}

// ============ DEPLOY_SENSOR ============
// 5 sensores = 5 passos, cada sensor = 20% progresso
MissionRecord *deploy_sensor(MissionRecord *mission)
{
    mission->x1 = 15.0;
    mission->y1 = 15.0;
    mission->x2 = 35.0;
    mission->y2 = 35.0;
    mission->duration = 500;

    mission->progress = 0;

    print_timestamp();
    printf("[MISSÃO] DEPLOY_SENSOR: 5 sensores a implantar\n");
    printf("   Zona: (%.1f,%.1f) -> (%.1f,%.1f)\n",
           mission->x1, mission->y1, mission->x2, mission->y2);
    printf("   Progresso: +20%% por cada sensor\n");

    return mission;
}

// ============ FUNÇÃO GENÉRICA ============
// Executar a missão correta baseada no tipo de tarefa
MissionRecord *execute_mission_logic(MissionRecord *mission, const char *task_type)
{
    if (!mission || !task_type)
        return NULL;

    if (strcmp(task_type, "capture_images") == 0)
    {
        return capture_images(mission);
    }
    else if (strcmp(task_type, "analyze_soil") == 0)
    {
        return analyze_soil(mission);
    }
    else if (strcmp(task_type, "collect_samples") == 0)
    {
        return collect_samples(mission);
    }
    else if (strcmp(task_type, "scan_area") == 0)
    {
        return scan_area(mission);
    }
    else if (strcmp(task_type, "deploy_sensor") == 0)
    {
        return deploy_sensor(mission);
    }

    return NULL;
}

// Enviar atualização de progresso
void send_progress_update(int sockfd, struct sockaddr_in *server_addr,
                          RoverState *state, uint8_t progress, uint8_t battery)
{
    Packet pkt;
    prepare_progress_packet(state, &pkt, progress, battery);

    print_timestamp();
    printf("📤 Enviando PROGRESS (seq=%u | progr=%u%% | bat=%u%%)\n",
           pkt.seq, progress, battery);

    ssize_t result = send_udp_with_ack(sockfd, server_addr, (char *)&pkt, sizeof(pkt));

    if (result < 0)
    {
        print_timestamp();
        printf("✗ Falha ao enviar PROGRESS\n\n");
    }
    else
    {
        print_timestamp();
        printf("✓ PROGRESS enviada com sucesso\n\n");

        // ===== GUARDAR ESTADO A CADA PROGRESSO =====
        state->battery = battery;
        state->progress = progress; save_rover_state(state->rover_id, state, 0, 0); } }

void recharge_battery(uint8_t *bateria) {
   print_timestamp();
    printf("🔋 ALERTA: Bateria baixa! (%u%%)\n", *bateria);
    printf("🔌 Iniciando recarregamento em campo...\n");
    
    // Aguardar 15 segundos
    for (int i = 15; i > 0; i--)
    {
        print_timestamp();
        printf("⏳ Recarregando... %d segundos restantes\n", i);
        sleep(1);
    }
    
    // Bateria volta a 100%
    *bateria = 100;
    
    print_timestamp();
    printf("✓ Recarregamento completo! Bateria: 100%%\n");
    printf("▶ Continuando missão...\n\n"); 
}

void execute_mission_progress(int sockfd, struct sockaddr_in *server_addr,
                              RoverState *state, uint8_t *progress, uint8_t *battery)
{
    while (*progress < 100)
    {
        *progress += 20;
        *battery -= 15;
        if (*progress > 100)
            *progress = 100;
        if (*battery < 10)
            *battery = 10;

        // ===== NOVO: VERIFICAR BATERIA ANTES DE CONTINUAR =====
        if (*battery < 20 && *progress < 100)
        {
            print_timestamp();
            printf("🔋 Bateria crítica detectada (%.0u%%)\n\n", *battery);
            recharge_battery(battery);
        }

        if (*progress < 100)
        {
            // Enviar progresso
            send_progress_update(sockfd, server_addr, state, *progress, *battery);
            next_sequence(state);

            // ===== AGUARDAR COM PROCESSAMENTO DE PING =====
            print_timestamp();
            printf("⏱ Aguardando %u segundos antes do próximo update...\n",
                   state->update_interval);

            // Dividir o sleep em pedaços pequenos para processar PINGs
            for (unsigned int i = 0; i < state->update_interval; i++)
            {
                sleep(1);

                // Verificar se há PING (com timeout muito curto)
                struct timeval tv = {0, 100000}; // 100ms
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(sockfd, &readfds);

                int rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);
                if (rv > 0 && FD_ISSET(sockfd, &readfds))
                {
                    Packet ping_pkt;
                    socklen_t addr_len = sizeof(*server_addr);

                    int r = recvfrom(sockfd, &ping_pkt, sizeof(ping_pkt), 0,
                                     (struct sockaddr *)server_addr, &addr_len);

                    if (r == sizeof(Packet) && ping_pkt.type == PKT_PING)
                    {
                        print_timestamp();
                        printf("\n🏓 PING recebido - Respondendo...\n");
                        process_ping_and_respond(sockfd, &ping_pkt, server_addr, state->rover_id);
                        print_timestamp();
                        printf("⏱ Continuando aguardar (%u segundos restantes)...\n",
                               state->update_interval - i - 1);
                    }
                }
            }

            printf("\n");
        }
    }
}
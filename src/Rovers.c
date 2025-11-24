// ============ Rovers.c (COM TELEMETRYSTREAM) ============
// Cliente MissionLink + TelemetryStream
#include "MissionLink.h"
#include "rover_management.h"
#include "executar_missoes.h"
#include "rover_state_persistence.h"
#include "Heartbeat.h"
#include "TelemetryStream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/select.h>

// Variáveis globais para tracking de posição
float current_position_x = 0.0;
float current_position_y = 0.0;
time_t last_telemetry_send = 0;

// Conectar ao servidor com handshake
int connect_to_server(int sockfd, struct sockaddr_in *server_addr)
{
    struct timeval tv = {HANDSHAKE_TIMEOUT, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    Packet hello;
    memset(&hello, 0, sizeof(hello));
    hello.type = 0xFF;

    char ack;
    socklen_t addr_len = sizeof(*server_addr);
    int attempts = 0;

    print_timestamp();
    printf("🤝 Iniciando handshake com servidor...\n");

    while (attempts < HANDSHAKE_RETRIES)
    {
        sendto(sockfd, &hello, 1, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));

        int r = recvfrom(sockfd, &ack, 1, 0, (struct sockaddr *)server_addr, &addr_len);
        if (r == 1 && ack == '1')
        {
            print_timestamp();
            printf("✓ Handshake aceito! Conexão estabelecida.\n\n");
            return 1;
        }

        attempts++;
        print_timestamp();
        printf("⚠  Tentativa %d/%d falhou. Retentando em 2s...\n",
               attempts, HANDSHAKE_RETRIES);
        sleep(2);
    }

    print_timestamp();
    printf("✗ Falha ao conectar ao servidor. Encerrando.\n");
    return 0;
}

// Solicitar missão
void request_mission(int sockfd, struct sockaddr_in *server_addr, RoverState *state)
{
    Packet request;
    prepare_request_packet(state, &request);

    print_timestamp();
    printf("📤 Enviando MISSION_REQUEST (seq=%u)\n", request.seq);

    ssize_t result = send_udp_with_ack(sockfd, server_addr, (char *)&request, sizeof(request));

    if (result < 0)
    {
        print_timestamp();
        printf("✗ Falha ao enviar MISSION_REQUEST\n\n");
    }
    else
    {
        print_timestamp();
        printf("✓ MISSION_REQUEST enviada com sucesso\n\n");
    }
}

// Receber atribuição de missão
char *receive_mission_assignment(int sockfd, struct sockaddr_in *server_addr, RoverState *state)
{
    static Packet assignment;
    memset(&assignment, 0, sizeof(assignment));

    struct timeval tv = {5, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    socklen_t addr_len = sizeof(*server_addr);
    ssize_t r = recvfrom(sockfd, &assignment, sizeof(assignment), 0,
                         (struct sockaddr *)server_addr, &addr_len);

    if (r == sizeof(Packet) && assignment.type == PKT_MISSION_ASSIGN)
    {
        print_timestamp();
        printf("🔨 MISSION_ASSIGN recebida\n");
        print_packet_info(&assignment);
        printf("\n");

        store_mission_assignment(state, &assignment);
        next_sequence(state);
        return assignment.task_type;
    }

    print_timestamp();
    printf("✗ Falha ao receber atribuição de missão\n\n");
    return "";
}

// Enviar conclusão de missão
void send_mission_complete(int sockfd, struct sockaddr_in *server_addr,
                           RoverState *state, uint8_t battery)
{
    Packet pkt;
    prepare_complete_packet(state, &pkt, battery);

    print_timestamp();
    printf("📤 Enviando COMPLETE (seq=%u | bat=%u%%)\n", pkt.seq, battery);

    ssize_t result = send_udp_with_ack(sockfd, server_addr, (char *)&pkt, sizeof(pkt));

    if (result < 0)
    {
        print_timestamp();
        printf("✗ Falha ao enviar COMPLETE\n\n");
    }
    else
    {
        print_timestamp();
        printf("✓ COMPLETE enviada com sucesso\n\n");
        save_rover_state(state->rover_id, state, current_position_x, current_position_y);
    }
}

// Simular movimento do rover
void simulate_rover_movement(float *pos_x, float *pos_y)
{
    float delta_x = (rand() % 10 - 5) * 0.5;
    float delta_y = (rand() % 10 - 5) * 0.5;
    
    *pos_x += delta_x;
    *pos_y += delta_y;
}

// Preparar mensagem de telemetria
void prepare_telemetry_message(TelemetryMessage *msg, RoverState *state,
                               float pos_x, float pos_y)
{
    memset(msg, 0, sizeof(*msg));
    
    msg->timestamp = (uint32_t)time(NULL);
    strncpy(msg->rover_id, state->rover_id, sizeof(msg->rover_id) - 1);
    msg->position_x = pos_x;
    msg->position_y = pos_y;
    msg->battery = state->battery;
    msg->state = (state->has_mission ? STATE_IN_MISSION : STATE_IDLE);
    msg->temperature = 25.0 + (rand() % 10);
    msg->signal_strength = 85 + (rand() % 15);
    msg->nonce = rand() % 100000;
}

// Enviar telemetria periodicamente
void send_telemetry_if_needed(int telemetry_fd, RoverState *state,
                              float pos_x, float pos_y)
{
    time_t now = time(NULL);
    
    if ((now - last_telemetry_send) >= TELEMETRY_SEND_INTERVAL)
    {
        TelemetryMessage msg;
        prepare_telemetry_message(&msg, state, pos_x, pos_y);
        send_telemetry_message(telemetry_fd, &msg);
        last_telemetry_send = now;
    }
}

// ============ VERSÃO COMPLETA: Rovers.c com telemetria contínua ============
// Adicione esta versão melhorada do main

int main(int argc, char **argv)
{
    srand(time(NULL));

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <rover_id>\n", argv[0]);
        return 1;
    }

    // ===== SOCKET UDP (MissionLink) =====
    int sockfd = create_udp_socket();
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // ===== SOCKET TCP (TelemetryStream) =====
    int telemetry_fd = create_telemetry_connection("127.0.0.1", TELEMETRY_PORT);
    if (telemetry_fd < 0) {
        print_timestamp();
        printf("⚠  Aviso: Não foi possível conectar ao TelemetryStream\n");
        printf("   Continuando apenas com MissionLink...\n\n");
    }

    // Conectar ao servidor MissionLink
    if (!connect_to_server(sockfd, &server_addr))
    {
        close(sockfd);
        if (telemetry_fd > 0) close(telemetry_fd);
        return 1;
    }

    init_state_file(argv[1]);

    RoverState state;
    init_rover_state(&state, argv[1]);

    if (load_rover_state(argv[1], &state, &current_position_x, &current_position_y))
    {
        print_timestamp();
        printf("✓ Estado anterior restaurado com sucesso!\n\n");
        state.seq++;
    }
    else
    {
        print_timestamp();
        printf("🔄 Iniciando com posição padrão: (%.2f, %.2f)\n\n",
               current_position_x, current_position_y);
    }

    print_timestamp();
    printf("👊 Sistema de Heartbeat ativado - Responderá a PINGs\n");
    printf("📡 Telemetria ativada - Enviando a cada %d segundos\n\n", TELEMETRY_SEND_INTERVAL);

    last_telemetry_send = time(NULL);
    time_t last_mission_request_time = time(NULL);

    int mission_num = 1;
    int in_mission = 0;

    while (1)
    {
        // ===== VERIFICAR SE É HORA DE ENVIAR TELEMETRIA =====
        time_t now = time(NULL);
        if (telemetry_fd > 0 && (now - last_telemetry_send) >= TELEMETRY_SEND_INTERVAL)
        {
            TelemetryMessage msg;
            uint8_t current_state = in_mission ? STATE_IN_MISSION : STATE_IDLE;
            
            memset(&msg, 0, sizeof(msg));
            msg.timestamp = (uint32_t)now;
            strncpy(msg.rover_id, state.rover_id, sizeof(msg.rover_id) - 1);
            msg.position_x = current_position_x;
            msg.position_y = current_position_y;
            msg.battery = state.battery;
            msg.state = current_state;
            msg.temperature = 25.0 + (rand() % 10);
            msg.signal_strength = 85 + (rand() % 15);
            msg.nonce = rand() % 100000;
            
            send_telemetry_message(telemetry_fd, &msg);
            last_telemetry_send = now;
        }

        // ===== SE NÃO ESTÁ EM MISSÃO, SOLICITAR UMA =====
        if (!in_mission && (now - last_mission_request_time) >= 5)
        {
            print_timestamp();
            printf("┌────────────────────────────────────────────────────┐\n");
            printf("│         🚀 CICLO DE MISSÃO #%d                      │\n", mission_num);
            printf("└────────────────────────────────────────────────────┘\n\n");

            request_mission(sockfd, &server_addr, &state);
            next_sequence(&state);
            
            // Receber atribuição
            char *task_type;
            task_type = receive_mission_assignment(sockfd, &server_addr, &state);
            if (!task_type || strlen(task_type) == 0)
            {
                break;
            }

            printf("A missao é %s\n\n", task_type);
            
            in_mission = 1;
            last_mission_request_time = now;
        }

        // ===== SE ESTÁ EM MISSÃO, EXECUTAR =====
        if (in_mission)
        {
            // Usar select com timeout pequeno para não bloquear
            struct timeval tv = {1, 0};  // 1 segundo timeout
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);
            
            int rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);
            
            // Se há dados UDP, processar (é resposta a PING)
            if (rv > 0 && FD_ISSET(sockfd, &readfds))
            {
                Packet ping_pkt;
                socklen_t addr_len = sizeof(server_addr);
                int r = recvfrom(sockfd, &ping_pkt, sizeof(ping_pkt), 0,
                               (struct sockaddr *)&server_addr, &addr_len);
                
                if (r == sizeof(Packet) && ping_pkt.type == PKT_PING)
                {
                    print_timestamp();
                    printf("🔔 PING recebido - Respondendo com PONG\n\n");
                    process_ping_and_respond(sockfd, &ping_pkt, &server_addr, state.rover_id);
                }
            }
            
            // Simular progresso (na prática seria substituído pela lógica real)
            static uint8_t mission_progress = 0;
            static uint8_t mission_battery = 100;
            
            if (mission_progress < 100)
            {
                mission_progress += 20;
                mission_battery -= 15;
                if (mission_progress > 100) mission_progress = 100;
                if (mission_battery < 10) mission_battery = 10;
                
                if (mission_progress < 100)
                {
                    Packet pkt;
                    prepare_progress_packet(&state, &pkt, mission_progress, mission_battery);
                    ssize_t result = send_udp_with_ack(sockfd, &server_addr, (char *)&pkt, sizeof(pkt));
                    
                    if (result > 0)
                    {
                        print_timestamp();
                        printf("✓ PROGRESS enviada (progr=%u%%, bat=%u%%)\n\n", mission_progress, mission_battery);
                    }
                    
                    state.battery = mission_battery;
                    state.progress = mission_progress;
                    next_sequence(&state);
                    sleep(2);  // Aguardar entre updates
                }
                else
                {
                    // Missão completa
                    Packet complete_pkt;
                    prepare_complete_packet(&state, &complete_pkt, mission_battery);
                    send_udp_with_ack(sockfd, &server_addr, (char *)&complete_pkt, sizeof(complete_pkt));
                    
                    print_timestamp();
                    printf("✅ COMPLETE enviada\n\n");
                    
                    state.battery = mission_battery;
                    state.progress = 100;
                    next_sequence(&state);
                    save_rover_state(state.rover_id, &state, current_position_x, current_position_y);
                    
                    // Reset para próxima missão
                    in_mission = 0;
                    mission_progress = 0;
                    mission_battery = 100;
                    state.battery = 100;
                    state.progress = 0;
                    mission_num++;
                    
                    print_timestamp();
                    printf("📋 Rover retornando à base...\n");
                    printf("⏱ Aguardando antes da próxima missão...\n\n");
                    sleep(3);
                }
            }
        }
    }

    print_timestamp();
    printf("┌────────────────────────────────────────────────────┐\n");
    printf("│       ✅ ROVER COMPLETOU TODAS AS MISSÕES ✅          │\n");
    printf("└────────────────────────────────────────────────────┘\n\n");

    save_rover_state(state.rover_id, &state, current_position_x, current_position_y);

    if (telemetry_fd > 0) close(telemetry_fd);
    close(sockfd);
    return 0;
}
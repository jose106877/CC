// ============ Nave-Mae.c (COM API DE OBSERVAÇÃO) ============
// Servidor MissionLink + TelemetryStream + API REST
#include "MissionLink.h"
#include "Server_management.h"
#include "Heartbeat.h"
#include "TelemetryStream.h"
#include "API_Observation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

extern RoverSession sessions[MAX_ROVERS];
extern int num_sessions;
extern MissionRecord missions[MAX_MISSIONS];
extern int num_missions;

int find_rover_index(const char *rover_id)
{
    for (int i = 0; i < num_sessions; i++)
    {
        if (strcmp(sessions[i].rover_id, rover_id) == 0)
        {
            return i;
        }
    }
    return -1;
}

void handle_mission_request(int sockfd, Packet *buffer, struct sockaddr_in *client_addr,
                            socklen_t addr_len, HeartbeatState *heartbeat_states)
{
    print_timestamp();
    printf("🔨 MISSION_REQUEST recebido\n");
    print_packet_info(buffer);

    send_ack_packet(sockfd, client_addr, buffer->seq);
    print_timestamp();
    printf("✓ ACK enviado\n\n");

    RoverSession *rover = register_or_update_rover(buffer->rover_id, client_addr);
    if (!rover)
    {
        print_timestamp();
        printf("✗ Não foi possível registar rover\n\n");
        return;
    }

    MissionRecord *mission = create_mission_for_rover(buffer->rover_id);
    if (!mission)
        return;

    Packet assign;
    memset(&assign, 0, sizeof(assign));
    assign.type = PKT_MISSION_ASSIGN;
    assign.seq = buffer->seq + 1;
    assign.battery = 100;
    assign.progress = 0;
    assign.nonce = rand() % 100000;

    strncpy(assign.rover_id, buffer->rover_id, sizeof(assign.rover_id) - 1);
    strncpy(assign.mission_id, mission->mission_id, sizeof(assign.mission_id) - 1);
    strncpy(assign.task_type, mission->task_type, sizeof(assign.task_type) - 1);

    assign.mission_x1 = mission->x1;
    assign.mission_y1 = mission->y1;
    assign.mission_x2 = mission->x2;
    assign.mission_y2 = mission->y2;
    assign.mission_duration = mission->duration;
    assign.update_interval = mission->update_interval;

    ssize_t sent = sendto(sockfd, &assign, sizeof(assign), 0,
                          (struct sockaddr *)client_addr, addr_len);

    if (sent > 0)
    {
        print_timestamp();
        printf("🔤 MISSION_ASSIGN enviada\n");
        print_packet_info(&assign);
        printf("\n");

        rover->last_seq = assign.seq;
        strncpy(rover->mission_id, mission->mission_id, sizeof(rover->mission_id) - 1);
        strncpy(rover->task_type, mission->task_type, sizeof(rover->task_type) - 1);

        print_mission_status();
        print_rover_status();
    }
}

void handle_progress(int sockfd, Packet *buffer, struct sockaddr_in *client_addr,
                     socklen_t addr_len, HeartbeatState *heartbeat_states)
{
    print_timestamp();
    printf("🔨 PROGRESS recebido\n");
    print_packet_info(buffer);

    send_ack_packet(sockfd, client_addr, buffer->seq);
    print_timestamp();
    printf("✓ ACK enviado\n\n");

    RoverSession *rover = register_or_update_rover(buffer->rover_id, client_addr);
    if (!rover)
        return;

    if (buffer->seq > rover->last_seq)
    {
        rover->last_seq = buffer->seq;
        rover->battery = buffer->battery;
        rover->progress = buffer->progress;
        rover->last_update = time(NULL);

        int idx = find_rover_index(buffer->rover_id);
        if (idx >= 0)
        {
            heartbeat_states[idx].last_pong_received = time(NULL);
            heartbeat_states[idx].consecutive_missed_pongs = 0;
            heartbeat_states[idx].is_healthy = 1;
        }

        add_or_update_mission(buffer->mission_id, buffer->progress, buffer->battery);
        print_mission_status();
        print_rover_status();
    }
}

void handle_complete(int sockfd, Packet *buffer, struct sockaddr_in *client_addr,
                     socklen_t addr_len, HeartbeatState *heartbeat_states)
{
    print_timestamp();
    printf("🔨 COMPLETE recebido\n");
    print_packet_info(buffer);

    send_ack_packet(sockfd, client_addr, buffer->seq);
    print_timestamp();
    printf("✓ ACK enviado\n\n");

    RoverSession *rover = register_or_update_rover(buffer->rover_id, client_addr);
    if (!rover)
        return;

    if (buffer->seq > rover->last_seq)
    {
        rover->last_seq = buffer->seq;
        rover->battery = buffer->battery;
        rover->progress = 100;
        rover->last_update = time(NULL);

        int idx = find_rover_index(buffer->rover_id);
        if (idx >= 0)
        {
            heartbeat_states[idx].last_pong_received = time(NULL);
            heartbeat_states[idx].consecutive_missed_pongs = 0;
            heartbeat_states[idx].is_healthy = 1;
        }

        mark_mission_complete(buffer->mission_id);
        add_or_update_mission(buffer->mission_id, 100, buffer->battery);

        print_timestamp();
        printf("✅ MISSÃO CONCLUÍDA: %s\n\n", buffer->mission_id);

        print_mission_status();
        print_rover_status();
    }
}

void handle_pong(Packet *buffer, struct sockaddr_in *client_addr,
                 HeartbeatState *heartbeat_states)
{
    print_timestamp();
    printf("🔔 PONG recebido de %s\n", buffer->rover_id);

    RoverSession *rover = register_or_update_rover(buffer->rover_id, client_addr);
    if (rover)
    {
        int idx = find_rover_index(buffer->rover_id);
        if (idx >= 0)
        {
            process_pong(rover, &heartbeat_states[idx]);
        }
    }
}

int main(void)
{
    srand(time(NULL));

    // ===== SOCKET UDP (MissionLink) =====
    int sockfd = create_udp_socket();
    struct sockaddr_in server_addr, client_addr;
    bind_udp_socket(sockfd, PORT, &server_addr);

    // ===== SOCKET TCP (TelemetryStream) =====
    int telemetry_fd = create_telemetry_server(TELEMETRY_PORT);
    if (telemetry_fd < 0)
    {
        print_timestamp();
        printf("❌ Erro ao criar servidor TelemetryStream\n");
        close(sockfd);
        return 1;
    }

    // ===== SOCKET HTTP (API de Observação) =====
    int api_fd = create_http_server(API_PORT);
    if (api_fd < 0)
    {
        print_timestamp();
        printf("❌ Erro ao criar servidor HTTP\n");
        close(sockfd);
        close(telemetry_fd);
        return 1;
    }

    init_server_tables();

    Packet buffer;
    HeartbeatState heartbeat_states[MAX_ROVERS];
    memset(heartbeat_states, 0, sizeof(heartbeat_states));

    TelemetrySession telemetry_sessions[MAX_TELEMETRY_CONNECTIONS];
    int telemetry_count = 0;
    memset(telemetry_sessions, 0, sizeof(telemetry_sessions));

    // ===== HTTP CLIENTS =====
    int api_clients[MAX_HTTP_CLIENTS];
    int api_client_count = 0;
    memset(api_clients, 0, sizeof(api_clients));

    time_t last_heartbeat_check = time(NULL);

    print_timestamp();
    printf("🚀 Servidor Nave-Mãe iniciado\n");
    printf("   MissionLink (UDP): porta %d\n", PORT);
    printf("   TelemetryStream (TCP): porta %d\n", TELEMETRY_PORT);
    printf("   API Observação (HTTP): porta %d\n", API_PORT);
    printf("🚀 Aguardando conexões de rovers...\n");
    printf("🔔 Sistema de Heartbeat ativado (intervalo: %d segundos)\n\n", HEARTBEAT_INTERVAL);

    while (1)
    {
        socklen_t addr_len = sizeof(client_addr);

        // ===== SELECT: Monitorizar todos os sockets =====
        struct timeval tv = {1, 0};
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);       // UDP
        FD_SET(telemetry_fd, &readfds); // TCP Telemetria
        FD_SET(api_fd, &readfds);       // HTTP API

        int max_fd = sockfd > telemetry_fd ? sockfd : telemetry_fd;
        max_fd = max_fd > api_fd ? max_fd : api_fd;

        // Adicionar sockets ativos de telemetria
        for (int i = 0; i < telemetry_count; i++)
        {
            if (telemetry_sessions[i].sockfd > 0)
            {
                FD_SET(telemetry_sessions[i].sockfd, &readfds);
                if (telemetry_sessions[i].sockfd > max_fd)
                {
                    max_fd = telemetry_sessions[i].sockfd;
                }
            }
        }

        // Adicionar sockets ativos de HTTP
        for (int i = 0; i < api_client_count; i++)
        {
            if (api_clients[i] > 0)
            {
                FD_SET(api_clients[i], &readfds);
                if (api_clients[i] > max_fd)
                {
                    max_fd = api_clients[i];
                }
            }
        }

        int rv = select(max_fd + 1, &readfds, NULL, NULL, &tv);

        time_t now = time(NULL);
        if (now - last_heartbeat_check >= HEARTBEAT_INTERVAL)
        {
            print_timestamp();
            printf("🔔 Verificando saúde dos rovers...\n");
            check_and_send_heartbeats(sockfd, sessions, num_sessions);
            print_heartbeat_status(sessions, num_sessions);
            print_telemetry_status(telemetry_sessions, telemetry_count);
            last_heartbeat_check = now;
        }

        if (rv <= 0)
            continue;

        // ===== ACEITAR CONEXÃO HTTP =====
        if (FD_ISSET(api_fd, &readfds) && api_client_count < MAX_HTTP_CLIENTS)
        {
            int client_fd = accept_http_connection(api_fd);
            if (client_fd > 0)
            {
                api_clients[api_client_count++] = client_fd;
                print_timestamp();
                printf("🌐 Nova requisição HTTP\n");
            }
        }

        // ============ TRECHO A SUBSTITUIR EM Nave-Mae.c ============
        // Substitua o loop HTTP por este:

        // ===== ACEITAR NOVA CONEXÃO HTTP =====
        if (FD_ISSET(api_fd, &readfds))
        {
            int client_fd = accept_http_connection(api_fd);
            if (client_fd > 0)
            {
                // Processar requisição HTTP imediatamente
                char request_buf[2048];
                memset(request_buf, 0, sizeof(request_buf));

                int n = recv(client_fd, request_buf, sizeof(request_buf) - 1, 0);

                if (n > 0)
                {
                    request_buf[n] = '\0';

                    // DEBUG: Ver requisição
                    print_timestamp();
                    printf("🌐 HTTP Request recebido (%d bytes)\n", n);

                    // IMPORTANTE: Passar os dados ATUAIS (não cópias de cache)
                    process_http_request(client_fd, request_buf,
                                         sessions, num_sessions, // Referências ao array real
                                         missions, num_missions, // Referências ao array real
                                         telemetry_sessions, telemetry_count);
                }

                close(client_fd);
            }
        }

        // ===== ACEITAR CONEXÃO TCP (TelemetryStream) =====
        if (FD_ISSET(telemetry_fd, &readfds))
        {
            accept_telemetry_connection(telemetry_fd, telemetry_sessions, &telemetry_count);
        }

        // ===== RECEBER DADOS DE TELEMETRIA =====
        for (int i = 0; i < telemetry_count; i++)
        {
            if (telemetry_sessions[i].sockfd > 0 && FD_ISSET(telemetry_sessions[i].sockfd, &readfds))
            {
                receive_telemetry_data(&telemetry_sessions[i]);
            }
        }

        // ===== RECEBER PACOTES UDP (MissionLink) =====
        if (FD_ISSET(sockfd, &readfds))
        {
            int r = recvfrom(sockfd, &buffer, sizeof(buffer), 0,
                             (struct sockaddr *)&client_addr, &addr_len);
            if (r <= 0)
                continue;

            if (buffer.type == 0xFF)
            {
                char ack = '1';
                sendto(sockfd, &ack, 1, 0, (struct sockaddr *)&client_addr, addr_len);
                print_timestamp();
                printf("🤝 Handshake recebido\n\n");
                continue;
            }

            switch (buffer.type)
            {
            case PKT_PONG:
                handle_pong(&buffer, &client_addr, heartbeat_states);
                break;
            case PKT_MISSION_REQUEST:
                handle_mission_request(sockfd, &buffer, &client_addr, addr_len, heartbeat_states);
                break;
            case PKT_PROGRESS:
                handle_progress(sockfd, &buffer, &client_addr, addr_len, heartbeat_states);
                break;
            case PKT_COMPLETE:
                handle_complete(sockfd, &buffer, &client_addr, addr_len, heartbeat_states);
                break;
            default:
                break;
            }
        }
    }

    close(sockfd);
    close(telemetry_fd);
    close(api_fd);
    return 0;
}
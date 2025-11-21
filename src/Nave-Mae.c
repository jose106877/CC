// ============ Nave-Mae_main.c (Completo) ============
// Servidor MissionLink: Nave-Mãe com Sistema de Heartbeat
#include "MissionLink.h"
#include "Server_management.h"
#include "Heartbeat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

// Variáveis globais para acesso às tabelas de rovers
extern RoverSession sessions[MAX_ROVERS];
extern int num_sessions;

// Encontrar índice do rover na tabela
int find_rover_index(const char *rover_id) {
    for (int i = 0; i < num_sessions; i++) {
        if (strcmp(sessions[i].rover_id, rover_id) == 0) {
            return i;
        }
    }
    return -1;
}

int main(void) {
    srand(time(NULL));
    
    int sockfd = create_udp_socket();
    struct sockaddr_in server_addr, client_addr;
    bind_udp_socket(sockfd, PORT, &server_addr);

    init_server_tables();

    Packet buffer;
    
    // Rastrear heartbeat de cada rover
    HeartbeatState heartbeat_states[MAX_ROVERS];
    memset(heartbeat_states, 0, sizeof(heartbeat_states));
    
    time_t last_heartbeat_check = time(NULL);

    print_timestamp();
    printf("🚀 Servidor Nave-Mãe iniciado na porta %d\n", PORT);
    printf("🚀 Aguardando conexões de rovers...\n");
    printf("💓 Sistema de Heartbeat ativado (intervalo: %d segundos)\n\n", HEARTBEAT_INTERVAL);

    while (1) {
        socklen_t addr_len = sizeof(client_addr);
        
        // Usar select para não ficar bloqueado
        struct timeval tv = {5, 0};
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        
        int rv = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        
        // ===== VERIFICAR HEARTBEAT PERIODICAMENTE =====
        time_t now = time(NULL);
        if (now - last_heartbeat_check >= HEARTBEAT_INTERVAL) {
            print_timestamp();
            printf("💓 Verificando saúde dos rovers...\n");
            check_and_send_heartbeats(sockfd, sessions, num_sessions);
            print_heartbeat_status(sessions, num_sessions);
            last_heartbeat_check = now;
        }
        
        // ===== RECEBER PACOTES =====
        if (rv > 0 && FD_ISSET(sockfd, &readfds)) {
            int r = recvfrom(sockfd, &buffer, sizeof(buffer), 0, 
                            (struct sockaddr *)&client_addr, &addr_len);
            if (r <= 0) continue;

            // ===== HANDSHAKE =====
            if (buffer.type == 0xFF) {
                char ack = '1';
                sendto(sockfd, &ack, 1, 0, (struct sockaddr *)&client_addr, addr_len);
                print_timestamp();
                printf("🤝 Handshake recebido\n\n");
                continue;
            }

            // ===== PONG (Heartbeat Response) =====
            if (buffer.type == PKT_PONG) {
                print_timestamp();
                printf("💓 PONG recebido de %s\n", buffer.rover_id);
                
                RoverSession *rover = register_or_update_rover(buffer.rover_id, &client_addr);
                if (rover) {
                    int idx = find_rover_index(buffer.rover_id);
                    if (idx >= 0) {
                        process_pong(rover, &heartbeat_states[idx]);
                    }
                }
                continue;
            }

            // ===== MISSION REQUEST =====
            if (buffer.type == PKT_MISSION_REQUEST) {
                print_timestamp();
                printf("📨 MISSION_REQUEST recebido\n");
                print_packet_info(&buffer);
                
                // Enviar ACK
                send_ack_packet(sockfd, &client_addr, buffer.seq);
                print_timestamp();
                printf("✓ ACK enviado\n\n");
                
                // Registar ou atualizar rover
                RoverSession *rover = register_or_update_rover(buffer.rover_id, &client_addr);
                if (!rover) {
                    print_timestamp();
                    printf("✗ Não foi possível registar rover\n\n");
                    continue;
                }
                
                // Criar missão
                MissionRecord *mission = create_mission_for_rover(buffer.rover_id);
                if (!mission) {
                    continue;
                }
                
                // Preparar MISSION_ASSIGN
                Packet assign;
                memset(&assign, 0, sizeof(assign));
                assign.type = PKT_MISSION_ASSIGN;
                assign.seq = buffer.seq + 1;
                assign.battery = 100;
                assign.progress = 0;
                assign.nonce = rand() % 100000;
                
                strncpy(assign.rover_id, buffer.rover_id, sizeof(assign.rover_id) - 1);
                strncpy(assign.mission_id, mission->mission_id, sizeof(assign.mission_id) - 1);
                strncpy(assign.task_type, mission->task_type, sizeof(assign.task_type) - 1);
                
                assign.mission_x1 = mission->x1;
                assign.mission_y1 = mission->y1;
                assign.mission_x2 = mission->x2;
                assign.mission_y2 = mission->y2;
                assign.mission_duration = mission->duration;
                assign.update_interval = mission->update_interval;
                
                // Enviar MISSION_ASSIGN
                ssize_t sent = sendto(sockfd, &assign, sizeof(assign), 0,
                                     (struct sockaddr *)&client_addr, addr_len);
                
                if (sent > 0) {
                    print_timestamp();
                    printf("📤 MISSION_ASSIGN enviada\n");
                    print_packet_info(&assign);
                    printf("\n");
                    
                    rover->last_seq = assign.seq;
                    strncpy(rover->mission_id, mission->mission_id, sizeof(rover->mission_id) - 1);
                    strncpy(rover->task_type, mission->task_type, sizeof(rover->task_type) - 1);
                    print_mission_status();
                    print_rover_status();
                }
                continue;
            }

            // ===== PROGRESS =====
            if (buffer.type == PKT_PROGRESS) {
                print_timestamp();
                printf("📨 PROGRESS recebido\n");
                print_packet_info(&buffer);
                
                send_ack_packet(sockfd, &client_addr, buffer.seq);
                print_timestamp();
                printf("✓ ACK enviado\n\n");
                
                RoverSession *rover = register_or_update_rover(buffer.rover_id, &client_addr);
                if (!rover) continue;
                
                if (buffer.seq > rover->last_seq) {
                    rover->last_seq = buffer.seq;
                    rover->battery = buffer.battery;
                    rover->progress = buffer.progress;
                    rover->last_update = time(NULL);
                    
                    // Atualizar heartbeat
                    int idx = find_rover_index(buffer.rover_id);
                    if (idx >= 0) {
                        heartbeat_states[idx].last_pong_received = time(NULL);
                        heartbeat_states[idx].consecutive_missed_pongs = 0;
                        heartbeat_states[idx].is_healthy = 1;
                    }
                    
                    add_or_update_mission(buffer.mission_id, buffer.progress, buffer.battery);
                    print_mission_status();
                    print_rover_status();
                } else {
                    print_timestamp();
                    printf("⚠ Pacote duplicado (seq=%u, esperado >%u)\n\n", 
                           buffer.seq, rover->last_seq);
                }
                continue;
            }

            // ===== COMPLETE =====
            if (buffer.type == PKT_COMPLETE) {
                print_timestamp();
                printf("📨 COMPLETE recebido\n");
                print_packet_info(&buffer);
                
                send_ack_packet(sockfd, &client_addr, buffer.seq);
                print_timestamp();
                printf("✓ ACK enviado\n\n");
                
                RoverSession *rover = register_or_update_rover(buffer.rover_id, &client_addr);
                if (!rover) continue;
                
                if (buffer.seq > rover->last_seq) {
                    rover->last_seq = buffer.seq;
                    rover->battery = buffer.battery;
                    rover->progress = 100;
                    rover->last_update = time(NULL);
                    
                    // Atualizar heartbeat
                    int idx = find_rover_index(buffer.rover_id);
                    if (idx >= 0) {
                        heartbeat_states[idx].last_pong_received = time(NULL);
                        heartbeat_states[idx].consecutive_missed_pongs = 0;
                        heartbeat_states[idx].is_healthy = 1;
                    }
                    
                    mark_mission_complete(buffer.mission_id);
                    add_or_update_mission(buffer.mission_id, 100, buffer.battery);
                    
                    print_timestamp();
                    printf("✅ MISSÃO CONCLUÍDA: %s\n\n", buffer.mission_id);
                    
                    print_mission_status();
                    print_rover_status();
                } else {
                    print_timestamp();
                    printf("⚠ Pacote duplicado (seq=%u, esperado >%u)\n\n", 
                           buffer.seq, rover->last_seq);
                }
                continue;
            }
        }
    }

    close(sockfd);
    return 0;
}
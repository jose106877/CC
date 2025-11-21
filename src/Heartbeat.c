// ============ Heartbeat.c (COMPLETO) ============
// Sistema de heartbeat com detecção adequada de rovers inativos
#include "Heartbeat.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>

// ============ SERVIDOR (NAVE-MÃE) ============

// Verificar e enviar PING para rovers
void check_and_send_heartbeats(int sockfd, RoverSession *sessions, int num_rovers) {
    time_t now = time(NULL);
    
    for (int i = 0; i < num_rovers; i++) {
        if (!sessions[i].active) continue;
        
        // ===== VERIFICAR TIMEOUT DE PONG =====
        // Se estávamos à espera de PONG e passou muito tempo
        if (sessions[i].waiting_for_pong) {
            time_t time_since_ping = now - sessions[i].last_ping_sent;
            
            if (time_since_ping > HEARTBEAT_TIMEOUT) {
                sessions[i].consecutive_missed_pongs++;
                
                print_timestamp();
                printf("⏱️  TIMEOUT de PONG de %s (tentativa %d/%d)\n",
                       sessions[i].rover_id,
                       sessions[i].consecutive_missed_pongs,
                       HEARTBEAT_MAX_RETRIES);
                
                // Se excedeu retentativas, marcar como inativo
                if (sessions[i].consecutive_missed_pongs > HEARTBEAT_MAX_RETRIES) {
                    sessions[i].active = 0;
                    print_timestamp();
                    printf("💀 Rover %s marcado como INATIVO (sem resposta)\n\n",
                           sessions[i].rover_id);
                } else {
                    // Tentar enviar novo PING
                    sessions[i].waiting_for_pong = 0;
                }
            } else {
                // Ainda aguardando, não fazer nada
                continue;
            }
        }
        
        // ===== ENVIAR NOVO PING =====
        // Se não estamos à espera de PONG, verificar se é hora de enviar
        if (!sessions[i].waiting_for_pong) {
            time_t time_since_last_update = now - sessions[i].last_update;
            
            // Enviar PING a cada HEARTBEAT_INTERVAL segundos
            if (time_since_last_update >= HEARTBEAT_INTERVAL) {
                print_timestamp();
                printf("💓 Enviando PING para %s (verificação de saúde)...\n",
                       sessions[i].rover_id);
                
                // Preparar pacote PING
                Packet ping;
                memset(&ping, 0, sizeof(ping));
                ping.type = PKT_PING;
                ping.seq = sessions[i].last_seq + 1;
                strncpy(ping.rover_id, sessions[i].rover_id, sizeof(ping.rover_id) - 1);
                
                // Enviar PING
                ssize_t sent = sendto(sockfd, &ping, sizeof(ping), 0,
                                     (struct sockaddr *)&sessions[i].addr,
                                     sizeof(sessions[i].addr));
                
                if (sent > 0) {
                    // Marcar que estamos à espera de PONG
                    sessions[i].waiting_for_pong = 1;
                    sessions[i].last_ping_sent = now;
                    
                    print_timestamp();
                    printf("   ✓ PING enviado\n");
                } else {
                    print_timestamp();
                    printf("   ✗ Erro ao enviar PING\n");
                }
            }
        }
    }
}

// Processar PONG recebido
void process_pong(RoverSession *rover, HeartbeatState *heartbeat) {
    heartbeat->last_pong_received = time(NULL);
    heartbeat->consecutive_missed_pongs = 0;
    heartbeat->is_healthy = 1;
    rover->active = 1;
    rover->waiting_for_pong = 0;  // Deixou de esperar
    rover->last_update = time(NULL);
    
    print_timestamp();
    printf("💓 PONG recebido de %s - Rover SAUDÁVEL ✓\n\n",
           rover->rover_id);
}

// Marcar rover como inativo
void mark_rover_inactive(RoverSession *rover, HeartbeatState *heartbeat) {
    heartbeat->is_healthy = 0;
    rover->active = 0;
    
    print_timestamp();
    printf("💀 Rover %s INATIVO - Nenhuma resposta após %d PINGs\n",
           rover->rover_id, HEARTBEAT_MAX_RETRIES);
    print_timestamp();
    printf("   Última atividade: %lds atrás\n\n",
           time(NULL) - heartbeat->last_pong_received);
}

// Imprimir status de heartbeat
void print_heartbeat_status(RoverSession *sessions, int num_rovers) {
    print_timestamp();
    printf("\n╔═══════════════════════════════════════════════════════╗\n");
    printf("║              💓 STATUS DE HEARTBEAT                      ║\n");
    printf("╠═══════════════════════════════════════════════════════╣\n");
    printf("║ Rover      │ Status       │ Última Resposta │ À Espera ║\n");
    printf("╠═══════════════════════════════════════════════════════╣\n");
    
    if (num_rovers == 0) {
        printf("║ Nenhum rover conectado                                 ║\n");
    } else {
        for (int i = 0; i < num_rovers; i++) {
            if (!sessions[i].active) continue;
            
            time_t time_since_update = time(NULL) - sessions[i].last_update;
            const char *status = sessions[i].active ? "✓ SAUDÁVEL" : "✗ INATIVO";
            const char *waiting = sessions[i].waiting_for_pong ? "SIM" : "NÃO";
            
            printf("║ %-10s │ %-12s │ %3lds atrás      │ %s    ║\n",
                   sessions[i].rover_id,
                   status,
                   time_since_update,
                   waiting);
        }
    }
    printf("╚═══════════════════════════════════════════════════════╝\n\n");
}

// ============ CLIENTE (ROVER) ============

// Processar PING e enviar PONG
void process_ping_and_respond(int sockfd, Packet *ping_pkt,
                             struct sockaddr_in *server_addr,
                             const char *rover_id) {
    print_timestamp();
    printf("💓 PING recebido da Nave-Mãe - Respondendo com PONG...\n");
    
    // Preparar pacote PONG
    Packet pong;
    memset(&pong, 0, sizeof(pong));
    pong.type = PKT_PONG;
    pong.seq = ping_pkt->seq;  // Manter a mesma sequência
    
    strncpy(pong.rover_id, rover_id, sizeof(pong.rover_id) - 1);
    
    // Enviar PONG
    ssize_t sent = sendto(sockfd, &pong, sizeof(pong), 0,
                         (struct sockaddr *)server_addr,
                         sizeof(*server_addr));
    
    if (sent > 0) {
        print_timestamp();
        printf("✓ PONG enviado com sucesso\n\n");
    } else {
        print_timestamp();
        printf("✗ Erro ao enviar PONG\n\n");
    }
}
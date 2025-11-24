// ============ TelemetryStream.c ============
// Implementação do protocolo de telemetria via TCP
#include "TelemetryStream.h"
#include "MissionLink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// ============ SERVIDOR (NAVE-MÃE) ============

// Criar socket servidor TCP na porta de telemetria
int create_telemetry_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket telemetria");
        return -1;
    }
    
    // Permitir reutilização da porta
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind telemetria");
        close(fd);
        return -1;
    }
    
    if (listen(fd, MAX_TELEMETRY_CONNECTIONS) < 0) {
        perror("listen telemetria");
        close(fd);
        return -1;
    }
    
    print_timestamp();
    printf("📡 Servidor TelemetryStream ligado na porta %d\n", port);
    printf("📡 Aguardando conexões de telemetria dos rovers...\n\n");
    
    return fd;
}

// Aceitar nova conexão TCP
void accept_telemetry_connection(int server_fd, TelemetrySession *sessions, int *count) {
    if (*count >= MAX_TELEMETRY_CONNECTIONS) {
        print_timestamp();
        printf("⚠  Limite de conexões telemetria atingido\n");
        return;
    }
    
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("accept telemetria");
        return;
    }
    
    // Registar nova sessão
    TelemetrySession *session = &sessions[*count];
    memset(session, 0, sizeof(*session));
    session->sockfd = client_fd;
    session->addr = client_addr;
    session->active = 1;
    session->last_update = time(NULL);
    (*count)++;
    
    print_timestamp();
    printf("✅ Nova conexão telemetria aceita (%d/%d)\n", *count, MAX_TELEMETRY_CONNECTIONS);
    printf("   IP: %s | Porto: %d\n\n",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
}

// Receber dados de telemetria
void receive_telemetry_data(TelemetrySession *session) {
    TelemetryMessage msg;
    memset(&msg, 0, sizeof(msg));
    
    ssize_t n = recv(session->sockfd, (char*)&msg, sizeof(TelemetryMessage), 0);
    
    if (n <= 0) {
        // Conexão fechada ou erro
        print_timestamp();
        printf("❌ Conexão telemetria perdida: %s\n", session->rover_id);
        close(session->sockfd);
        session->sockfd = 0;
        session->active = 0;
        return;
    }
    
    // Armazenar dados
    strncpy(session->rover_id, msg.rover_id, sizeof(session->rover_id) - 1);
    session->last_position_x = msg.position_x;
    session->last_position_y = msg.position_y;
    session->last_battery = msg.battery;
    session->last_state = msg.state;
    session->last_temperature = msg.temperature;
    session->last_signal_strength = msg.signal_strength;
    session->last_update = time(NULL);
    
    // Imprimir para debug
    const char *state_str[] = {"IDLE", "IN_MISSION", "RETURNING", "ERROR", "CHARGING"};
    const char *state_name = (msg.state < 5) ? state_str[msg.state] : "UNKNOWN";
    
    print_timestamp();
    printf("[TELEMETRY] %s\n", msg.rover_id);
    printf("   Posição: (%.2f, %.2f)\n", msg.position_x, msg.position_y);
    printf("   Bateria: %u%% | Temp: %.1f°C | Sinal: %u%%\n",
           msg.battery, msg.temperature, msg.signal_strength);
    printf("   Estado: %s | Nonce: %u\n\n", state_name, msg.nonce);
}

// Imprimir status de todas as conexões de telemetria
void print_telemetry_status(TelemetrySession *sessions, int count) {
    print_timestamp();
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║              📡 ESTADO DAS TELEMETRIAS                        ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║ Rover      │ Posição        │ Bat  │ Temp  │ Sinal │ Ativo   ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    if (count == 0) {
        printf("║ Nenhuma telemetria ativa                                       ║\n");
    } else {
        for (int i = 0; i < count; i++) {
            if (!sessions[i].active) continue;
            
            time_t time_since_update = time(NULL) - sessions[i].last_update;
            const char *status = (time_since_update < 10) ? "✓ OK" : "⚠ LENTO";
            
            printf("║ %-10s │ (%.1f, %.1f)    │ %3u%% │ %.1f° │ %3u%% │ %s  ║\n",
                   sessions[i].rover_id,
                   sessions[i].last_position_x,
                   sessions[i].last_position_y,
                   sessions[i].last_battery,
                   sessions[i].last_temperature,
                   sessions[i].last_signal_strength,
                   status);
        }
    }
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");
}

// ============ CLIENTE (ROVER) ============

// Conectar ao servidor de telemetria
int create_telemetry_connection(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket telemetria cliente");
        return -1;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(fd);
        return -1;
    }
    
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect telemetria");
        close(fd);
        return -1;
    }
    
    print_timestamp();
    printf("✅ Conectado ao servidor TelemetryStream (%s:%d)\n\n", host, port);
    
    return fd;
}

// Preparar e enviar mensagem de telemetria
void send_telemetry_message(int fd, TelemetryMessage *msg) {
    if (fd < 0) return;
    
    msg->timestamp = (uint32_t)time(NULL);
    msg->nonce = rand() % 100000;
    
    ssize_t sent = send(fd, (char*)msg, sizeof(TelemetryMessage), 0);
    
    if (sent < 0) {
        print_timestamp();
        printf("❌ Erro ao enviar telemetria\n");
    } else if (sent == sizeof(TelemetryMessage)) {
        print_timestamp();
        printf("[SENT] 📡 Telemetria enviada: %s\n", msg->rover_id);
        printf("   Pos: (%.2f, %.2f) | Bat: %u%% | Temp: %.1f°C\n\n",
               msg->position_x, msg->position_y, msg->battery, msg->temperature);
    } else {
        print_timestamp();
        printf("⚠  Envio parcial de telemetria (%zd/%zu bytes)\n\n", sent, sizeof(TelemetryMessage));
    }
}
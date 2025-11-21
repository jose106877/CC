// ============ Rover_main.c (Completo com Heartbeat) ============
// Cliente MissionLink: Rover com Sistema de Heartbeat
#include "MissionLink.h"
#include "rover_management.h"
#include "executar_missoes.h"
#include "rover_state_persistence.h"
#include "Heartbeat.h"
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
        printf("⚠ Tentativa %d/%d falhou. Retentando em 2s...\n",
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
        printf("📨 MISSION_ASSIGN recebida\n");
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
        
        // Guardar estado final
        save_rover_state(state->rover_id, state, current_position_x, current_position_y);
    }
}

// Simular movimento do rover
void simulate_rover_movement(float *pos_x, float *pos_y)
{
    float delta_x = (rand() % 10 - 5) * 0.5;  // -2.5 a +2.5
    float delta_y = (rand() % 10 - 5) * 0.5;
    
    *pos_x += delta_x;
    *pos_y += delta_y;
}

int main(int argc, char **argv)
{
    srand(time(NULL));

    int sockfd = create_udp_socket();
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // Conectar ao servidor
    if (!connect_to_server(sockfd, &server_addr))
    {
        close(sockfd);
        return 1;
    }

    init_state_file(argv[1]);

    // Inicializar estado do rover
    RoverState state;
    init_rover_state(&state, argv[1]);

    // ===== TENTAR CARREGAR ESTADO ANTERIOR =====
    if (load_rover_state(argv[1], &state, &current_position_x, &current_position_y))
    {
        print_timestamp();
        printf("✓ Estado anterior restaurado com sucesso!\n\n");
        state.seq++;  // Incrementar sequência para continuar
    }
    else
    {
        print_timestamp();
        printf("📍 Iniciando com posição padrão: (%.2f, %.2f)\n\n",
               current_position_x, current_position_y);
    }

    print_timestamp();
    printf("💟 Sistema de Heartbeat ativado - Responderá a PINGs\n\n");

    // Loop: executar 3 ciclos de missão
    int mission_num = 1;
    while (1)
    {
        print_timestamp();
        printf("┌───────────────────────────────────────────────────────┐\n");
        printf("│         🚀 CICLO DE MISSÃO #%d                      │\n", mission_num);
        printf("└───────────────────────────────────────────────────────┘\n\n");

        // 1. Solicitar missão
        request_mission(sockfd, &server_addr, &state);
        next_sequence(&state);
        sleep(1);

        // 2. Receber atribuição
        char *task_type;
        task_type = receive_mission_assignment(sockfd, &server_addr, &state);
        if (!task_type || strlen(task_type) == 0)
        {
            break;
        }

        printf("A missao é %s\n\n", task_type);

        // 3. Executar missão com atualizações
        uint8_t progress = 0;
        uint8_t battery = state.battery;
        execute_mission_progress(sockfd, &server_addr, &state, &progress, &battery);

        // 4. Enviar conclusão;
        send_mission_complete(sockfd, &server_addr, &state, battery);
        next_sequence(&state);

        // Preparar para próxima missão
        
        print_timestamp();
        printf("🔋 Rover retornando à base para recarregar...\n");
        printf("⏱ Aguardando 5 segundos antes da próxima missão...\n\n");
        sleep(5);
        battery = 100;
        state.has_mission = 0;
        mission_num++;
    }

    print_timestamp();
    printf("┌───────────────────────────────────────────────────────┐\n");
    printf("│       ✅ ROVER COMPLETOU TODAS AS MISSÕES ✅          │\n");
    printf("└───────────────────────────────────────────────────────┘\n\n");

    save_rover_state(state.rover_id, &state, current_position_x, current_position_y);

    close(sockfd);
    return 0;
}
// ============ MissionLink.c ============
#include "MissionLink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int create_udp_socket(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void bind_udp_socket(int sockfd, int port, struct sockaddr_in *addr) {
    memset(addr, 0, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)addr, sizeof(*addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
}

int receive_udp(int sockfd, char *buffer, size_t buf_size, 
                struct sockaddr_in *client_addr) {
    socklen_t addr_len = sizeof(*client_addr);
    int received = recvfrom(sockfd, buffer, buf_size, 0, 
                           (struct sockaddr *)client_addr, &addr_len);
    return received;
}

void send_ack_packet(int sockfd, struct sockaddr_in *addr, uint32_t seq) {
    Packet ack_pkt;
    memset(&ack_pkt, 0, sizeof(ack_pkt));
    ack_pkt.type = PKT_ACK;
    ack_pkt.seq = seq;
    
    sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
           (struct sockaddr *)addr, sizeof(*addr));
}

ssize_t send_udp_with_ack(int sockfd, struct sockaddr_in *server_addr, 
                          char *data, size_t size) {
    Packet ack_pkt;
    ssize_t sent = sendto(sockfd, data, size, 0, 
                         (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (sent < 0) return -1;

    struct timeval tv = {1, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    int retries = 0;
    socklen_t addr_len = sizeof(*server_addr);
    
    while (retries < 5) {
        int r = recvfrom(sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
                        (struct sockaddr *)server_addr, &addr_len);
        
        if (r == sizeof(Packet) && ack_pkt.type == PKT_ACK) {
            print_timestamp();
            printf("[UDP] ✓ ACK recebido (seq=%u)\n", ack_pkt.seq);
            return sent;
        }
        
        retries++;
        print_timestamp();
        printf("[UDP] ⚠ ACK não recebido, retentando... (%d/5)\n", retries);
        usleep(200000);
        sendto(sockfd, data, size, 0, (struct sockaddr *)server_addr, 
               sizeof(*server_addr));
    }
    
    print_timestamp();
    printf("[UDP] ✗ Falha ao enviar com ACK após 5 tentativas\n");
    return -1;
}

ssize_t send_udp_fragmented(int sockfd, struct sockaddr_in *server_addr, 
                            char *data, size_t size) {
    size_t offset = 0;
    int frag_num = 0;
    
    print_timestamp();
    printf("[UDP] 📦 Iniciando envio fragmentado (%zu bytes)\n", size);
    
    while (offset < size) {
        size_t chunk_size = (size - offset > FRAG_SIZE) ? FRAG_SIZE : size - offset;
        char frag[FRAG_SIZE + 1];
        
        frag[0] = (offset + chunk_size < size) ? 1 : 0;
        memcpy(frag + 1, data + offset, chunk_size);
        
        ssize_t sent = sendto(sockfd, frag, chunk_size + 1, 0, 
                             (struct sockaddr *)server_addr, sizeof(*server_addr));
        if (sent < 0) {
            print_timestamp();
            printf("[UDP] ✗ Erro ao enviar fragmento %d\n", frag_num);
            return -1;
        }
        
        print_timestamp();
        printf("[UDP] 📨 Fragmento %d enviado (%zu bytes) [%s]\n", 
               frag_num, chunk_size, frag[0] ? "mais fragmentos" : "último");
        
        offset += chunk_size;
        frag_num++;
        usleep(50000);
    }
    
    print_timestamp();
    printf("[UDP] ✓ Envio fragmentado concluído (%d fragmentos)\n", frag_num);
    return size;
}

size_t receive_udp_reassemble(int sockfd, char *buffer, size_t buf_size, 
                              struct sockaddr_in *client_addr) {
    char temp[FRAG_SIZE + 1];
    size_t total_received = 0;
    int frag_num = 0;
    
    print_timestamp();
    printf("[UDP] 📦 Iniciando recepção fragmentada\n");

    while (1) {
        int r = receive_udp(sockfd, temp, FRAG_SIZE + 1, client_addr);
        if (r <= 0) {
            print_timestamp();
            printf("[UDP] ✗ Erro ao receber fragmento\n");
            break;
        }

        int is_fragmented = temp[0];
        size_t chunk_size = r - 1;
        
        if (total_received + chunk_size > buf_size) {
            fprintf(stderr, "[UDP] ✗ Buffer insuficiente para reconstituir mensagem\n");
            break;
        }
        
        memcpy(buffer + total_received, temp + 1, chunk_size);
        total_received += chunk_size;
        
        print_timestamp();
        printf("[UDP] 📥 Fragmento %d recebido (%zu bytes) [%s]\n", 
               frag_num, chunk_size, is_fragmented ? "mais fragmentos" : "último");

        if (!is_fragmented) break;
        frag_num++;
    }
    
    print_timestamp();
    printf("[UDP] ✓ Recepção fragmentada concluída (%d fragmentos, %zu bytes totais)\n", 
           frag_num, total_received);
    return total_received;
}

void print_timestamp(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

const char* get_packet_type_name(uint8_t type) {
    switch(type) {
        case PKT_MISSION_REQUEST: return "MISSION_REQUEST";
        case PKT_MISSION_ASSIGN: return "MISSION_ASSIGN";
        case PKT_PROGRESS: return "PROGRESS";
        case PKT_COMPLETE: return "COMPLETE";
        case PKT_ACK: return "ACK";
        default: return "UNKNOWN";
    }
}

void print_packet_info(Packet *pkt) {
    print_timestamp();
    printf("[PKT] Pacote Recebido: %s\n", get_packet_type_name(pkt->type));
    printf("[PKT]   Rover ID:       %s\n", pkt->rover_id);
    printf("[PKT]   Mission ID:     %s\n", pkt->mission_id);
    printf("[PKT]   Task Type:      %s\n", pkt->task_type);
    printf("[PKT]   Seq:            %u\n", pkt->seq);
    printf("[PKT]   Battery:        %u%%\n", pkt->battery);
    printf("[PKT]   Progress:       %u%%\n", pkt->progress);
    printf("[PKT]   Nonce:          %u\n", pkt->nonce);
    
    if (pkt->type == PKT_MISSION_ASSIGN) {
        printf("[PKT]   --- Parâmetros da Missão ---\n");
        printf("[PKT]   Área:           (%.1f, %.1f) → (%.1f, %.1f)\n",
               pkt->mission_x1, pkt->mission_y1, pkt->mission_x2, pkt->mission_y2);
        printf("[PKT]   Duração:        %u segundos\n", pkt->mission_duration);
        printf("[PKT]   Intervalo:      %u segundos\n", pkt->update_interval);
    }
}
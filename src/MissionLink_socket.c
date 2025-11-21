// ============ MissionLink_socket.c ============
// Funções de comunicação UDP do protocolo MissionLink
#include "MissionLink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>

// Criar socket UDP
int create_udp_socket(void) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

// Vincular socket a porta
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

// Receber pacote UDP
int receive_udp(int sockfd, char *buffer, size_t buf_size, 
                struct sockaddr_in *client_addr) {
    socklen_t addr_len = sizeof(*client_addr);
    int received = recvfrom(sockfd, buffer, buf_size, 0, 
                           (struct sockaddr *)client_addr, &addr_len);
    return received;
}

// Enviar ACK
void send_ack_packet(int sockfd, struct sockaddr_in *addr, uint32_t seq) {
    Packet ack_pkt;
    memset(&ack_pkt, 0, sizeof(ack_pkt));
    ack_pkt.type = PKT_ACK;
    ack_pkt.seq = seq;
    
    sendto(sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
           (struct sockaddr *)addr, sizeof(*addr));
}

// Enviar pacote com ACK confirmado (retransmissão automática)
ssize_t send_udp_with_ack(int sockfd, struct sockaddr_in *server_addr, 
                          char *data, size_t size) {
    Packet ack_pkt;
    ssize_t sent = sendto(sockfd, data, size, 0, 
                         (struct sockaddr *)server_addr, sizeof(*server_addr));
    if (sent < 0) return -1;

    struct timeval tv = {ACK_TIMEOUT, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    int retries = 0;
    socklen_t addr_len = sizeof(*server_addr);
    
    while (retries < ACK_RETRIES) {
        int r = recvfrom(sockfd, &ack_pkt, sizeof(ack_pkt), 0, 
                        (struct sockaddr *)server_addr, &addr_len);
        
        if (r == sizeof(Packet) && ack_pkt.type == PKT_ACK && ack_pkt.seq == ((Packet*)data)->seq) {
            print_timestamp();
            printf("[UDP] ✓ ACK recebido (seq=%u)\n", ack_pkt.seq);
            return sent;
        }
        
        retries++;
        if (retries < ACK_RETRIES) {
            print_timestamp();
            printf("[UDP] ⚠ ACK não recebido, retentando... (%d/%d)\n", retries, ACK_RETRIES);
            usleep(200000);
            sendto(sockfd, data, size, 0, (struct sockaddr *)server_addr, 
                   sizeof(*server_addr));
        }
    }
    
    print_timestamp();
    printf("[UDP] ✗ Falha ao enviar com ACK após %d tentativas\n", ACK_RETRIES);
    return -1;
}
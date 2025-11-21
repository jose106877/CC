// ============ MissionLink_utils.c ============
// Funções utilitárias do protocolo MissionLink
#include "MissionLink.h"
#include <stdio.h>
#include <time.h>

// Imprimir timestamp HH:MM:SS
void print_timestamp(void) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
}

// Converter tipo de pacote para string
const char* get_packet_type_name(uint8_t type) {
    switch(type) {
        case PKT_MISSION_REQUEST: return "MISSION_REQUEST";
        case PKT_MISSION_ASSIGN:  return "MISSION_ASSIGN";
        case PKT_PROGRESS:        return "PROGRESS";
        case PKT_COMPLETE:        return "COMPLETE";
        case PKT_ACK:             return "ACK";
        default:                  return "UNKNOWN";
    }
}

// Imprimir informações do pacote
void print_packet_info(Packet *pkt) {
    print_timestamp();
    printf("[PKT] Pacote: %s\n", get_packet_type_name(pkt->type));
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
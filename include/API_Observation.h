// ============ API_Observation.h ============
// API de Observação - Expõe dados de missões e telemetria via HTTP
// Fornece informações em tempo real sobre o estado do sistema

#ifndef API_OBSERVATION_H
#define API_OBSERVATION_H

#include "Server_management.h"
#include "TelemetryStream.h"
#include <stdint.h>

// ============ CONSTANTES ============
#define API_PORT 8080
#define API_BUFFER_SIZE 65536
#define MAX_HTTP_CLIENTS 10

// ============ TIPOS DE ENDPOINTS ============
typedef enum {
    ENDPOINT_ROVERS_LIST,      // GET /api/rovers
    ENDPOINT_ROVER_STATUS,     // GET /api/rovers/{id}
    ENDPOINT_MISSIONS_LIST,    // GET /api/missions
    ENDPOINT_MISSION_STATUS,   // GET /api/missions/{id}
    ENDPOINT_TELEMETRY_LAST,   // GET /api/telemetry/latest
    ENDPOINT_TELEMETRY_ROVER,  // GET /api/telemetry/{rover_id}
    ENDPOINT_SYSTEM_STATUS,    // GET /api/system/status
    ENDPOINT_NOT_FOUND,        // 404
    ENDPOINT_INVALID           // 400
} APIEndpoint;

// ============ FUNÇÕES: SERVIDOR HTTP ============

// Criar socket servidor HTTP
int create_http_server(int port);

// Aceitar conexão HTTP
int accept_http_connection(int server_fd);

// Processar requisição HTTP
void process_http_request(int client_fd, const char *request,
                         RoverSession *rovers, int num_rovers,
                         MissionRecord *missions, int num_missions,
                         TelemetrySession *telemetry, int num_telemetry);

// Identificar tipo de endpoint
APIEndpoint parse_http_endpoint(const char *request, char *resource_id);

// ============ GERAÇÃO DE RESPOSTAS (JSON) ============

// Gerar JSON com lista de rovers
void generate_rovers_list_json(char *buffer, size_t buf_size,
                               RoverSession *rovers, int num_rovers);

// Gerar JSON com status de um rover
void generate_rover_status_json(char *buffer, size_t buf_size,
                                RoverSession *rover);

// Gerar JSON com lista de missões
void generate_missions_list_json(char *buffer, size_t buf_size,
                                 MissionRecord *missions, int num_missions);

// Gerar JSON com status de uma missão
void generate_mission_status_json(char *buffer, size_t buf_size,
                                  MissionRecord *mission);

// Gerar JSON com últimas telemetrias
void generate_telemetry_latest_json(char *buffer, size_t buf_size,
                                    TelemetrySession *telemetry, int num_telemetry);

// Gerar JSON com telemetria de um rover
void generate_telemetry_rover_json(char *buffer, size_t buf_size,
                                   TelemetrySession *telemetry);

// Gerar JSON com status do sistema
void generate_system_status_json(char *buffer, size_t buf_size,
                                 RoverSession *rovers, int num_rovers,
                                 MissionRecord *missions, int num_missions,
                                 TelemetrySession *telemetry, int num_telemetry);

// ============ UTILITÁRIOS ============

// Enviar resposta HTTP
void send_http_response(int client_fd, int status_code, const char *content_type,
                       const char *body);

// Obter nome do estado operacional
const char* get_rover_state_name(uint8_t state);

// Converter timestamp para string ISO
void format_timestamp(uint32_t ts, char *buffer, size_t size);

#endif // API_OBSERVATION_H
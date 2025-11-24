// ============ API_Observation.c (COMPLETO - SEM DUPLICATAS) ============
// Implementação da API de Observação (HTTP REST)
#include "API_Observation.h"
#include "MissionLink.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// ============ SERVIDOR HTTP ============

int create_http_server(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket HTTP");
        return -1;
    }
    
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind HTTP");
        close(fd);
        return -1;
    }
    
    if (listen(fd, MAX_HTTP_CLIENTS) < 0) {
        perror("listen HTTP");
        close(fd);
        return -1;
    }
    
    print_timestamp();
    printf("🌐 API de Observação ligada na porta %d\n", port);
    printf("🌐 Aceder em: http://localhost:%d/api/system/status\n\n", port);
    
    return fd;
}

int accept_http_connection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        perror("accept HTTP");
        return -1;
    }
    
    return client_fd;
}

// ============ PARSING HTTP ============

APIEndpoint parse_http_endpoint(const char *request, char *resource_id) {
    if (!request) return ENDPOINT_INVALID;
    
    memset(resource_id, 0, 256);
    
    if (strncmp(request, "GET /api/rovers HTTP", 20) == 0) {
        return ENDPOINT_ROVERS_LIST;
    }
    else if (strncmp(request, "GET /api/rovers/", 16) == 0) {
        sscanf(request, "GET /api/rovers/%s HTTP", resource_id);
        for (int i = 0; i < 256; i++) {
            if (resource_id[i] == ' ' || resource_id[i] == '\n' || resource_id[i] == '\r') {
                resource_id[i] = '\0';
                break;
            }
        }
        return ENDPOINT_ROVER_STATUS;
    }
    else if (strncmp(request, "GET /api/missions HTTP", 22) == 0) {
        return ENDPOINT_MISSIONS_LIST;
    }
    else if (strncmp(request, "GET /api/missions/", 18) == 0) {
        sscanf(request, "GET /api/missions/%s HTTP", resource_id);
        for (int i = 0; i < 256; i++) {
            if (resource_id[i] == ' ' || resource_id[i] == '\n' || resource_id[i] == '\r') {
                resource_id[i] = '\0';
                break;
            }
        }
        return ENDPOINT_MISSION_STATUS;
    }
    else if (strncmp(request, "GET /api/telemetry/latest HTTP", 30) == 0) {
        return ENDPOINT_TELEMETRY_LAST;
    }
    else if (strncmp(request, "GET /api/telemetry/", 19) == 0) {
        sscanf(request, "GET /api/telemetry/%s HTTP", resource_id);
        for (int i = 0; i < 256; i++) {
            if (resource_id[i] == ' ' || resource_id[i] == '\n' || resource_id[i] == '\r') {
                resource_id[i] = '\0';
                break;
            }
        }
        return ENDPOINT_TELEMETRY_ROVER;
    }
    else if (strncmp(request, "GET /api/system/status HTTP", 27) == 0) {
        return ENDPOINT_SYSTEM_STATUS;
    }
    
    return ENDPOINT_NOT_FOUND;
}

// ============ UTILITÁRIOS ============

const char* get_rover_state_name(uint8_t state) {
    switch(state) {
        case STATE_IDLE:       return "idle";
        case STATE_IN_MISSION: return "in_mission";
        case STATE_RETURNING:  return "returning";
        case STATE_ERROR:      return "error";
        case STATE_CHARGING:   return "charging";
        default:               return "unknown";
    }
}

void format_timestamp(uint32_t ts, char *buffer, size_t size) {
    if (ts == 0) {
        snprintf(buffer, size, "2024-01-01T00:00:00Z");
        return;
    }
    time_t t = (time_t)ts;
    struct tm *tm_info = localtime(&t);
    strftime(buffer, size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

// ============ GERAÇÃO DE JSON ============

void generate_rovers_list_json(char *buffer, size_t buf_size,
                               RoverSession *rovers, int num_rovers) {
    if (!buffer || !rovers) {
        snprintf(buffer, buf_size, "{\"rovers\": []}\n");
        return;
    }
    
    int len = 0;
    len += snprintf(buffer + len, buf_size - len, "{\n  \"rovers\": [\n");
    
    int first = 1;
    int count = 0;
    
    for (int i = 0; i < num_rovers; i++) {
        if (!rovers[i].active) continue;
        
        if (!first) len += snprintf(buffer + len, buf_size - len, ",\n");
        
        time_t now = time(NULL);
        time_t time_since = now - rovers[i].last_update;
        
        len += snprintf(buffer + len, buf_size - len,
            "    {\n"
            "      \"id\": \"%s\",\n"
            "      \"status\": \"%s\",\n"
            "      \"battery\": %u,\n"
            "      \"progress\": %u,\n"
            "      \"mission_id\": \"%s\",\n"
            "      \"last_update_seconds_ago\": %ld\n"
            "    }",
            rovers[i].rover_id,
            (time_since < 35) ? "active" : "inactive",
            rovers[i].battery,
            rovers[i].progress,
            rovers[i].mission_id[0] ? rovers[i].mission_id : "null",
            time_since);
        
        first = 0;
        count++;
    }
    
    len += snprintf(buffer + len, buf_size - len, "\n  ]\n}\n");
    
    print_timestamp();
    printf("[API] Rovers JSON: %d rovers, %d bytes\n", count, len);
}

void generate_rover_status_json(char *buffer, size_t buf_size,
                                RoverSession *rover) {
    if (!buffer || !rover) return;
    
    time_t now = time(NULL);
    time_t time_since = now - rover->last_update;
    
    snprintf(buffer, buf_size,
        "{\n"
        "  \"rover\": {\n"
        "    \"id\": \"%s\",\n"
        "    \"status\": \"%s\",\n"
        "    \"battery\": %u,\n"
        "    \"progress\": %u,\n"
        "    \"current_mission\": \"%s\",\n"
        "    \"current_task\": \"%s\",\n"
        "    \"last_sequence\": %u,\n"
        "    \"last_update_ago\": %ld,\n"
        "    \"address\": \"%s:%d\"\n"
        "  }\n"
        "}\n",
        rover->rover_id,
        (time_since < 35) ? "active" : "inactive",
        rover->battery,
        rover->progress,
        rover->mission_id[0] ? rover->mission_id : "none",
        rover->task_type[0] ? rover->task_type : "none",
        rover->last_seq,
        time_since,
        inet_ntoa(rover->addr.sin_addr),
        ntohs(rover->addr.sin_port));
}

void generate_missions_list_json(char *buffer, size_t buf_size,
                                 MissionRecord *missions, int num_missions) {
    if (!buffer || !missions) {
        snprintf(buffer, buf_size, "{\"missions\": []}\n");
        return;
    }
    
    int len = 0;
    len += snprintf(buffer + len, buf_size - len, "{\n  \"missions\": [\n");
    
    int first = 1;
    int count = 0;
    
    for (int i = 0; i < num_missions; i++) {
        char start_time[32];
        format_timestamp((uint32_t)missions[i].start_time, start_time, sizeof(start_time));
        
        if (!first) len += snprintf(buffer + len, buf_size - len, ",\n");
        
        len += snprintf(buffer + len, buf_size - len,
            "    {\n"
            "      \"id\": \"%s\",\n"
            "      \"rover_id\": \"%s\",\n"
            "      \"task_type\": \"%s\",\n"
            "      \"progress\": %u,\n"
            "      \"battery\": %u,\n"
            "      \"status\": \"%s\",\n"
            "      \"area\": {\"x1\": %.1f, \"y1\": %.1f, \"x2\": %.1f, \"y2\": %.1f},\n"
            "      \"duration_max\": %u,\n"
            "      \"start_time\": \"%s\",\n"
            "      \"updates_received\": %d\n"
            "    }",
            missions[i].mission_id,
            missions[i].rover_id,
            missions[i].task_type,
            missions[i].progress,
            missions[i].battery,
            missions[i].completed ? "completed" : "in_progress",
            missions[i].x1, missions[i].y1, missions[i].x2, missions[i].y2,
            missions[i].duration,
            start_time,
            missions[i].updates_count);
        
        first = 0;
        count++;
    }
    
    len += snprintf(buffer + len, buf_size - len, "\n  ]\n}\n");
    
    print_timestamp();
    printf("[API] Missions JSON: %d missões, %d bytes\n", count, len);
}

void generate_mission_status_json(char *buffer, size_t buf_size,
                                  MissionRecord *mission) {
    if (!buffer || !mission) return;
    
    char start_time[32];
    format_timestamp((uint32_t)mission->start_time, start_time, sizeof(start_time));
    
    snprintf(buffer, buf_size,
        "{\n"
        "  \"mission\": {\n"
        "    \"id\": \"%s\",\n"
        "    \"rover_id\": \"%s\",\n"
        "    \"task_type\": \"%s\",\n"
        "    \"progress\": %u,\n"
        "    \"battery\": %u,\n"
        "    \"status\": \"%s\",\n"
        "    \"area\": {\n"
        "      \"x1\": %.1f,\n"
        "      \"y1\": %.1f,\n"
        "      \"x2\": %.1f,\n"
        "      \"y2\": %.1f\n"
        "    },\n"
        "    \"duration_max_seconds\": %u,\n"
        "    \"start_time\": \"%s\",\n"
        "    \"updates_received\": %d\n"
        "  }\n"
        "}\n",
        mission->mission_id,
        mission->rover_id,
        mission->task_type,
        mission->progress,
        mission->battery,
        mission->completed ? "completed" : "in_progress",
        mission->x1, mission->y1, mission->x2, mission->y2,
        mission->duration,
        start_time,
        mission->updates_count);
}

void generate_telemetry_latest_json(char *buffer, size_t buf_size,
                                    TelemetrySession *telemetry, int num_telemetry) {
    if (!buffer || !telemetry) {
        snprintf(buffer, buf_size, "{\"telemetry\": []}\n");
        return;
    }
    
    int len = 0;
    len += snprintf(buffer + len, buf_size - len, "{\n  \"telemetry\": [\n");
    
    int first = 1;
    int count = 0;
    
    for (int i = 0; i < num_telemetry; i++) {
        if (!telemetry[i].active) continue;
        
        time_t now = time(NULL);
        time_t time_since = now - telemetry[i].last_update;
        
        if (!first) len += snprintf(buffer + len, buf_size - len, ",\n");
        
        len += snprintf(buffer + len, buf_size - len,
            "    {\n"
            "      \"rover_id\": \"%s\",\n"
            "      \"position\": {\"x\": %.2f, \"y\": %.2f},\n"
            "      \"battery\": %u,\n"
            "      \"temperature\": %.1f,\n"
            "      \"signal_strength\": %u,\n"
            "      \"state\": \"%s\",\n"
            "      \"last_update_ago\": %ld\n"
            "    }",
            telemetry[i].rover_id,
            telemetry[i].last_position_x,
            telemetry[i].last_position_y,
            telemetry[i].last_battery,
            telemetry[i].last_temperature,
            telemetry[i].last_signal_strength,
            get_rover_state_name(telemetry[i].last_state),
            time_since);
        
        first = 0;
        count++;
    }
    
    len += snprintf(buffer + len, buf_size - len, "\n  ]\n}\n");
    
    print_timestamp();
    printf("[API] Telemetry JSON: %d sessões, %d bytes\n", count, len);
}

void generate_telemetry_rover_json(char *buffer, size_t buf_size,
                                   TelemetrySession *telemetry) {
    if (!buffer || !telemetry) return;
    
    time_t now = time(NULL);
    time_t time_since = now - telemetry->last_update;
    
    snprintf(buffer, buf_size,
        "{\n"
        "  \"telemetry\": {\n"
        "    \"rover_id\": \"%s\",\n"
        "    \"position\": {\n"
        "      \"x\": %.2f,\n"
        "      \"y\": %.2f\n"
        "    },\n"
        "    \"battery\": %u,\n"
        "    \"temperature\": %.1f,\n"
        "    \"signal_strength\": %u,\n"
        "    \"state\": \"%s\",\n"
        "    \"last_update_ago\": %ld\n"
        "  }\n"
        "}\n",
        telemetry->rover_id,
        telemetry->last_position_x,
        telemetry->last_position_y,
        telemetry->last_battery,
        telemetry->last_temperature,
        telemetry->last_signal_strength,
        get_rover_state_name(telemetry->last_state),
        time_since);
}

void generate_system_status_json(char *buffer, size_t buf_size,
                                 RoverSession *rovers, int num_rovers,
                                 MissionRecord *missions, int num_missions,
                                 TelemetrySession *telemetry, int num_telemetry) {
    if (!buffer) return;
    
    int active_rovers = 0, active_missions = 0, active_telemetry = 0;
    
    if (rovers) {
        for (int i = 0; i < num_rovers; i++) {
            if (rovers[i].active && (time(NULL) - rovers[i].last_update) < 35) {
                active_rovers++;
            }
        }
    }
    
    if (missions) {
        for (int i = 0; i < num_missions; i++) {
            if (!missions[i].completed) active_missions++;
        }
    }
    
    if (telemetry) {
        for (int i = 0; i < num_telemetry; i++) {
            if (telemetry[i].active && (time(NULL) - telemetry[i].last_update) < 10) {
                active_telemetry++;
            }
        }
    }
    
    snprintf(buffer, buf_size,
        "{\n"
        "  \"system\": {\n"
        "    \"timestamp\": %ld,\n"
        "    \"rovers\": {\n"
        "      \"total\": %d,\n"
        "      \"active\": %d\n"
        "    },\n"
        "    \"missions\": {\n"
        "      \"total\": %d,\n"
        "      \"in_progress\": %d,\n"
        "      \"completed\": %d\n"
        "    },\n"
        "    \"telemetry\": {\n"
        "      \"sessions\": %d,\n"
        "      \"active\": %d\n"
        "    }\n"
        "  }\n"
        "}\n",
        time(NULL),
        num_rovers,
        active_rovers,
        num_missions,
        active_missions,
        (num_missions > 0) ? (num_missions - active_missions) : 0,
        num_telemetry,
        active_telemetry);
}

// ============ RESPOSTAS HTTP ============

void send_http_response(int client_fd, int status_code, const char *content_type,
                       const char *body) {
    if (client_fd < 0) return;
    
    char response[API_BUFFER_SIZE];
    int len = 0;
    
    const char *status_msg = (status_code == 200) ? "OK" :
                            (status_code == 404) ? "Not Found" :
                            (status_code == 400) ? "Bad Request" : "Error";
    
    len += snprintf(response + len, API_BUFFER_SIZE - len,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n"
        "\r\n",
        status_code, status_msg, content_type, strlen(body));
    
    if (len + strlen(body) < API_BUFFER_SIZE) {
        strncpy(response + len, body, API_BUFFER_SIZE - len - 1);
    }
    
    send(client_fd, response, strlen(response), 0);
}

// ============ PROCESSAMENTO DE REQUISIÇÕES ============

void process_http_request(int client_fd, const char *request,
                         RoverSession *rovers, int num_rovers,
                         MissionRecord *missions, int num_missions,
                         TelemetrySession *telemetry, int num_telemetry) {
    if (client_fd < 0 || !request) return;
    
    char resource_id[256];
    char body[API_BUFFER_SIZE];
    memset(body, 0, API_BUFFER_SIZE);
    
    APIEndpoint endpoint = parse_http_endpoint(request, resource_id);
    
    switch(endpoint) {
        case ENDPOINT_ROVERS_LIST:
            generate_rovers_list_json(body, sizeof(body), rovers, num_rovers);
            send_http_response(client_fd, 200, "application/json", body);
            break;
            
        case ENDPOINT_ROVER_STATUS: {
            if (!rovers) {
                snprintf(body, sizeof(body), "{\"error\": \"No rovers available\"}");
                send_http_response(client_fd, 404, "application/json", body);
                break;
            }
            RoverSession *rover = NULL;
            for (int i = 0; i < num_rovers; i++) {
                if (strcmp(rovers[i].rover_id, resource_id) == 0) {
                    rover = &rovers[i];
                    break;
                }
            }
            if (rover) {
                generate_rover_status_json(body, sizeof(body), rover);
                send_http_response(client_fd, 200, "application/json", body);
            } else {
                snprintf(body, sizeof(body), "{\"error\": \"Rover not found\"}");
                send_http_response(client_fd, 404, "application/json", body);
            }
            break;
        }
        
        case ENDPOINT_MISSIONS_LIST:
            generate_missions_list_json(body, sizeof(body), missions, num_missions);
            send_http_response(client_fd, 200, "application/json", body);
            break;
            
        case ENDPOINT_MISSION_STATUS: {
            if (!missions) {
                snprintf(body, sizeof(body), "{\"error\": \"No missions available\"}");
                send_http_response(client_fd, 404, "application/json", body);
                break;
            }
            MissionRecord *mission = NULL;
            for (int i = 0; i < num_missions; i++) {
                if (strcmp(missions[i].mission_id, resource_id) == 0) {
                    mission = &missions[i];
                    break;
                }
            }
            if (mission) {
                generate_mission_status_json(body, sizeof(body), mission);
                send_http_response(client_fd, 200, "application/json", body);
            } else {
                snprintf(body, sizeof(body), "{\"error\": \"Mission not found\"}");
                send_http_response(client_fd, 404, "application/json", body);
            }
            break;
        }
        
        case ENDPOINT_TELEMETRY_LAST:
            generate_telemetry_latest_json(body, sizeof(body), telemetry, num_telemetry);
            send_http_response(client_fd, 200, "application/json", body);
            break;
            
        case ENDPOINT_TELEMETRY_ROVER: {
            if (!telemetry) {
                snprintf(body, sizeof(body), "{\"error\": \"No telemetry available\"}");
                send_http_response(client_fd, 404, "application/json", body);
                break;
            }
            TelemetrySession *telem = NULL;
            for (int i = 0; i < num_telemetry; i++) {
                if (strcmp(telemetry[i].rover_id, resource_id) == 0) {
                    telem = &telemetry[i];
                    break;
                }
            }
            if (telem) {
                generate_telemetry_rover_json(body, sizeof(body), telem);
                send_http_response(client_fd, 200, "application/json", body);
            } else {
                snprintf(body, sizeof(body), "{\"error\": \"Telemetry not found\"}");
                send_http_response(client_fd, 404, "application/json", body);
            }
            break;
        }
        
        case ENDPOINT_SYSTEM_STATUS:
            generate_system_status_json(body, sizeof(body), rovers, num_rovers,
                                      missions, num_missions, telemetry, num_telemetry);
            send_http_response(client_fd, 200, "application/json", body);
            break;
            
        default:
            snprintf(body, sizeof(body),
                "{\n  \"error\": \"Endpoint not found\",\n"
                "  \"available_endpoints\": [\n"
                "    \"GET /api/system/status\",\n"
                "    \"GET /api/rovers\",\n"
                "    \"GET /api/rovers/{id}\",\n"
                "    \"GET /api/missions\",\n"
                "    \"GET /api/missions/{id}\",\n"
                "    \"GET /api/telemetry/latest\",\n"
                "    \"GET /api/telemetry/{rover_id}\"\n"
                "  ]\n"
                "}\n");
            send_http_response(client_fd, 404, "application/json", body);
            break;
    }
}
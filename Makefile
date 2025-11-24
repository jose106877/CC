# ============ MAKEFILE - Projeto MissionLink + TelemetryStream + API ============
CC = gcc
CFLAGS_BASE = -Wall -Wextra -Werror=format -Werror=implicit -pedantic -std=c99 -I./include -D_DEFAULT_SOURCE
CFLAGS_DEBUG = $(CFLAGS_BASE) -g -O0 -DDEBUG
CFLAGS_RELEASE = $(CFLAGS_BASE) -O2
CFLAGS = $(CFLAGS_RELEASE)

# ============ DIRETÓRIOS ============
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
ROVERS_DIR = rovers

# ============ FICHEIROS COMUNS ============
COMMON_SRC = $(SRC_DIR)/MissionLink_socket.c \
             $(SRC_DIR)/MissionLink_utils.c \
             $(SRC_DIR)/Heartbeat.c \
             $(SRC_DIR)/TelemetryStream.c \
             $(SRC_DIR)/API_Observation.c

COMMON_OBJ = $(OBJ_DIR)/MissionLink_socket.o \
             $(OBJ_DIR)/MissionLink_utils.o \
             $(OBJ_DIR)/Heartbeat.o \
             $(OBJ_DIR)/TelemetryStream.o \
             $(OBJ_DIR)/API_Observation.o

# ============ FICHEIROS SERVIDOR (Nave-Mãe) ============
SERVER_SRC = $(SRC_DIR)/Server_management.c \
             $(SRC_DIR)/rover_management.c \
             $(SRC_DIR)/executar_missoes.c \
             $(SRC_DIR)/salvar_estado.c \
             $(SRC_DIR)/Nave-Mae.c

SERVER_OBJ = $(OBJ_DIR)/Server_management.o \
             $(OBJ_DIR)/rover_management.o \
             $(OBJ_DIR)/executar_missoes.o \
             $(OBJ_DIR)/salvar_estado.o \
             $(OBJ_DIR)/Nave-Mae.o

# ============ FICHEIROS CLIENTE (Rover) ============
CLIENT_SRC = $(SRC_DIR)/rover_management.c \
             $(SRC_DIR)/executar_missoes.c \
             $(SRC_DIR)/salvar_estado.c \
             $(SRC_DIR)/Rovers.c

CLIENT_OBJ = $(OBJ_DIR)/rover_management.o \
             $(OBJ_DIR)/executar_missoes.o \
             $(OBJ_DIR)/salvar_estado.o \
             $(OBJ_DIR)/Rovers.o

# ============ TARGETS ============
TARGETS = $(BIN_DIR)/navemae $(BIN_DIR)/rover

# ============ TARGETS PRINCIPAIS ============
.PHONY: all clean debug release help

all: release

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: clean $(TARGETS)
	@echo "✓ Build DEBUG concluído"

release: CFLAGS = $(CFLAGS_RELEASE)
release: clean $(TARGETS)
	@echo "✓ Build RELEASE concluído"

# ============ COMPILAÇÃO DE OBJETOS ============
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  ✓ $<"

# ============ TARGETS EXECUTÁVEIS ============

$(BIN_DIR)/navemae: $(COMMON_OBJ) $(SERVER_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "  ✓ Servidor criado: $@"

$(BIN_DIR)/rover: $(COMMON_OBJ) $(CLIENT_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "  ✓ Cliente criado: $@"

# ============ LIMPEZA ============
clean:
	@echo "🧹 Limpando ficheiros temporários..."
	@rm -rf $(OBJ_DIR)
	@rm -rf $(BIN_DIR)
	@rm -rf $(ROVERS_DIR)/*
	@echo "✓ Limpeza concluída"

distclean: clean
	@echo "🧹 Limpeza completa..."
	@find . -name "*~" -delete
	@find . -name ".DS_Store" -delete
	@echo "✓ Limpeza completa concluída"

# ============ EXECUÇÃO ============
.PHONY: run-server run-client run-ground-control

run-server: release
	@echo "🚀 Iniciando servidor (Nave-Mãe)..."
	./$(BIN_DIR)/navemae

run-client: release
	@echo "🚀 Iniciando cliente (Rover R-001)..."
	./$(BIN_DIR)/rover R-001

run-ground-control:
	@echo "🌐 Iniciando Ground Control..."
	@python3 ground_control.py

run-ground-control-live:
	@echo "🌐 Iniciando Ground Control (Modo Contínuo)..."
	@python3 ground_control.py --live

run-tmux: release
	@echo "🚀 Iniciando servidor, cliente e Ground Control em tmux..."
	@tmux new-session -d -s ml_session -x 200 -y 50
	@tmux send-keys -t ml_session "cd $(PWD) && ./$(BIN_DIR)/navemae" Enter
	@tmux split-window -t ml_session -h
	@tmux send-keys -t ml_session "sleep 1 && cd $(PWD) && ./$(BIN_DIR)/rover R-001" Enter
	@tmux split-window -t ml_session -v
	@tmux send-keys -t ml_session "sleep 2 && cd $(PWD) && python3 ground_control.py --live" Enter
	@tmux attach -t ml_session

# ============ TESTES ============
.PHONY: test-api

test-api:
	@echo "🧪 Testando endpoints da API..."
	@echo ""
	@echo "📊 System Status:"
	@curl -s http://localhost:8080/api/system/status | python3 -m json.tool
	@echo ""
	@echo "🤖 Rovers:"
	@curl -s http://localhost:8080/api/rovers | python3 -m json.tool
	@echo ""
	@echo "🎯 Missions:"
	@curl -s http://localhost:8080/api/missions | python3 -m json.tool
	@echo ""
	@echo "📡 Telemetry:"
	@curl -s http://localhost:8080/api/telemetry/latest | python3 -m json.tool

# ============ INFO ============
.PHONY: info

info:
	@echo "••••••••••••••••••••••••••••••••••••••••••••••••••••••"
	@echo "  📋 PROJETO MISSIONLINK + TELEMETRYSTREAM + API"
	@echo "••••••••••••••••••••••••••••••••••••••••••••••••••••••"
	@echo ""
	@echo "  🔧 Compilação:"
	@echo "     make all        : Build release (padrão)"
	@echo "     make debug      : Build com debug"
	@echo "     make clean      : Remover obj/ e bin/"
	@echo ""
	@echo "  🚀 Execução:"
	@echo "     make run-server              : Executar Nave-Mãe"
	@echo "     make run-client              : Executar Rover"
	@echo "     make run-ground-control      : Executar Ground Control"
	@echo "     make run-ground-control-live : Ground Control (tempo real)"
	@echo "     make run-tmux                : Executar tudo (requer tmux)"
	@echo ""
	@echo "  📡 Protocolos:"
	@echo "     MissionLink:    UDP porta 5005"
	@echo "     TelemetryStream: TCP porta 5006"
	@echo "     API HTTP:       porta 8080"
	@echo ""
	@echo "  🧪 Testes:"
	@echo "     make test-api   : Testar endpoints da API"
	@echo ""
	@echo "  📚 API Endpoints:"
	@echo "     GET /api/system/status"
	@echo "     GET /api/rovers"
	@echo "     GET /api/rovers/{id}"
	@echo "     GET /api/missions"
	@echo "     GET /api/missions/{id}"
	@echo "     GET /api/telemetry/latest"
	@echo "     GET /api/telemetry/{rover_id}"
	@echo ""

help: info

.DEFAULT_GOAL := all
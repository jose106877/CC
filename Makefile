# ============ MAKEFILE - Projeto MissionLink + TelemetryStream ============
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
             $(SRC_DIR)/TelemetryStream.c

COMMON_OBJ = $(OBJ_DIR)/MissionLink_socket.o \
             $(OBJ_DIR)/MissionLink_utils.o \
             $(OBJ_DIR)/Heartbeat.o \
             $(OBJ_DIR)/TelemetryStream.o

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
.PHONY: run-server run-client run

run-server: release
	@echo "🚀 Iniciando servidor (Nave-Mãe)..."
	./$(BIN_DIR)/navemae

run-client: release
	@echo "🚀 Iniciando cliente (Rover R-001)..."
	./$(BIN_DIR)/rover R-001

run-tmux: release
	@echo "🚀 Iniciando servidor e cliente em tmux..."
	@tmux new-session -d -s ml_session -x 200 -y 50
	@tmux send-keys -t ml_session "cd $(PWD) && ./$(BIN_DIR)/navemae" Enter
	@tmux split-window -t ml_session -h
	@tmux send-keys -t ml_session "sleep 1 && cd $(PWD) && ./$(BIN_DIR)/rover R-001" Enter
	@tmux attach -t ml_session

# ============ VERIFICAÇÃO ============
.PHONY: check

check:
	@echo "🔍 Verificando sintaxe..."
	@for file in $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h; do \
		echo "  Verificando: $$file"; \
		$(CC) -fsyntax-only $(CFLAGS_BASE) $$file || exit 1; \
	done
	@echo "✓ Sintaxe OK"

# ============ INFO ============
.PHONY: info

info:
	@echo "••••••••••••••••••••••••••••••••••••••••••••••••••••••"
	@echo "  📋 PROJETO MISSIONLINK + TELEMETRYSTREAM"
	@echo "••••••••••••••••••••••••••••••••••••••••••••••••••••••"
	@echo ""
	@echo "  🔧 Targets:"
	@echo "     make all        : Build release (padrão)"
	@echo "     make debug      : Build com debug"
	@echo "     make clean      : Remover obj/ e bin/"
	@echo ""
	@echo "  🚀 Execução:"
	@echo "     make run-server : Executar Nave-Mãe"
	@echo "     make run-client : Executar Rover"
	@echo "     make run-tmux   : Executar ambos (requer tmux)"
	@echo ""
	@echo "  📡 Protocolos:"
	@echo "     MissionLink:    UDP porta 5005"
	@echo "     TelemetryStream: TCP porta 5006"
	@echo ""

help: info

.DEFAULT_GOAL := all
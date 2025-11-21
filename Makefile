# ============ MAKEFILE - Projeto MissionLink ============
# Compilação de servidor (Nave-Mãe) e cliente (Rover)

# ============ VARIÁVEIS DE COMPILAÇÃO ============
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

# ============ FICHEIROS FONTE ============
# Ficheiros comuns (protocolo e utilitários)
COMMON_SRC = $(SRC_DIR)/MissionLink_socket.c \
             $(SRC_DIR)/MissionLink_utils.c \
             $(SRC_DIR)/Heartbeat.c

COMMON_OBJ = $(OBJ_DIR)/MissionLink_socket.o \
             $(OBJ_DIR)/MissionLink_utils.o \
             $(OBJ_DIR)/Heartbeat.o

# Ficheiros do servidor (Nave-Mãe)
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


# Ficheiros do cliente (Rover)
CLIENT_SRC = $(SRC_DIR)/rover_management.c \
             $(SRC_DIR)/executar_missoes.c \
             $(SRC_DIR)/salvar_estado.c \
             $(SRC_DIR)/Rovers.c


CLIENT_OBJ = $(OBJ_DIR)/rover_management.o \
             $(OBJ_DIR)/executar_missoes.o \
             $(OBJ_DIR)/salvar_estado.o \
             $(OBJ_DIR)/Rovers.o



# ============ TARGETS EXECUTÁVEIS ============
TARGETS = $(BIN_DIR)/navemae $(BIN_DIR)/rover

# ============ TARGETS PRINCIPAIS ============
.PHONY: all clean debug release help

# Target padrão: build release
all: release

# Build debug (com símbolos de debug)
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: clean $(TARGETS)
	@echo "✓ Build DEBUG concluído"

# Build release (otimizado)
release: CFLAGS = $(CFLAGS_RELEASE)
release: clean $(TARGETS)
	@echo "✓ Build RELEASE concluído"

# ============ COMPILAÇÃO DE OBJETOS ============
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "  ✓ $<"

# ============ TARGETS EXECUTÁVEIS ============

# Servidor Nave-Mãe
$(BIN_DIR)/navemae: $(COMMON_OBJ) $(SERVER_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@
	@echo "  ✓ Servidor criado: $@"

# Cliente Rover
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

# Executar servidor
run-server: release
	@echo "🚀 Iniciando servidor (Nave-Mãe)..."
	./$(BIN_DIR)/navemae

# Executar cliente
run-client: release
	@echo "🚀 Iniciando cliente (Rover)..."
	./$(BIN_DIR)/rover

# Executar ambos em paralelo (requer tmux)
run-tmux: release
	@echo "🚀 Iniciando servidor e cliente em tmux..."
	@tmux new-session -d -s ml_session -x 200 -y 50
	@tmux send-keys -t ml_session "cd $(PWD) && ./$(BIN_DIR)/navemae" Enter
	@tmux split-window -t ml_session -h
	@tmux send-keys -t ml_session "sleep 1 && cd $(PWD) && ./$(BIN_DIR)/rover" Enter
	@tmux attach -t ml_session

# ============ INFORMAÇÕES ============
.PHONY: info

info:
	@echo "•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••"
	@echo "  📋 PROJETO MISSIONLINK - Informações"
	@echo "•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••"
	@echo ""
	@echo "  📁 Estrutura:"
	@echo "     - include/    : Ficheiros header (.h)"
	@echo "     - src/        : Ficheiros fonte (.c)"
	@echo "     - obj/        : Ficheiros compilados (.o)"
	@echo "     - bin/        : Executáveis"
	@echo ""
	@echo "  🎯 Targets:"
	@echo "     make all      : Build release (padrão)"
	@echo "     make debug    : Build com debug"
	@echo "     make clean    : Remover obj/ e bin/"
	@echo "     make distclean: Limpeza completa"
	@echo ""
	@echo "  🚀 Execução:"
	@echo "     make run-server : Executar Nave-Mãe"
	@echo "     make run-client : Executar Rover"
	@echo "     make run-tmux   : Executar ambos (requer tmux)"
	@echo ""
	@echo "  🔧 Compilação:"
	@echo "     Compilador: $(CC)"
	@echo "     Flags BASE: $(CFLAGS_BASE)"
	@echo "     Flags DEBUG: $(CFLAGS_DEBUG)"
	@echo "     Flags RELEASE: $(CFLAGS_RELEASE)"
	@echo ""
	@echo "•••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••••"

help: info

# ============ VERIFICAÇÃO DE SINTAXE ============
.PHONY: check

check:
	@echo "🔍 Verificando sintaxe..."
	@for file in $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h; do \
		echo "  Verificando: $$file"; \
		$(CC) -fsyntax-only $(CFLAGS_BASE) $$file || exit 1; \
	done
	@echo "✓ Sintaxe OK"

# ============ CONTAGEM DE LINHAS ============
.PHONY: lines

lines:
	@echo "📊 Estatísticas do código:"
	@echo ""
	@echo "  Headers (.h):"
	@wc -l $(INCLUDE_DIR)/*.h | tail -1
	@echo ""
	@echo "  Código fonte (.c):"
	@wc -l $(SRC_DIR)/*.c | tail -1
	@echo ""
	@echo "  Total:"
	@wc -l $(INCLUDE_DIR)/*.h $(SRC_DIR)/*.c | tail -1

# ============ DEPENDÊNCIAS (AUTO-GERADAS) ============
.PHONY: depend

depend:
	@echo "📋 Gerando dependências..."
	@$(CC) -MM $(CFLAGS_BASE) $(SRC_DIR)/*.c > dependencies.mk
	@echo "✓ Dependências geradas: dependencies.mk"

# ============ VALGRIND (VERIFICAÇÃO DE MEMÓRIA) ============
.PHONY: valgrind-server valgrind-client

valgrind-server: debug
	@echo "🔧 Executando servidor com valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose ./$(BIN_DIR)/navemae

valgrind-client: debug
	@echo "🔧 Executando cliente com valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all \
		--track-origins=yes --verbose ./$(BIN_DIR)/rover

# ============ INSTALAÇÃO ============
.PHONY: install

INSTALL_DIR ?= /usr/local/bin

install: release
	@echo "📦 Instalando executáveis em $(INSTALL_DIR)..."
	@mkdir -p $(INSTALL_DIR)
	@cp $(BIN_DIR)/navemae $(INSTALL_DIR)/
	@cp $(BIN_DIR)/rover $(INSTALL_DIR)/
	@chmod +x $(INSTALL_DIR)/navemae $(INSTALL_DIR)/rover
	@echo "✓ Instalação concluída"

# ============ DESINSTALAÇÃO ============
.PHONY: uninstall

uninstall:
	@echo "🗑️  Desinstalando de $(INSTALL_DIR)..."
	@rm -f $(INSTALL_DIR)/navemae $(INSTALL_DIR)/rover
	@echo "✓ Desinstalação concluída"

# ============ TARGETS PARA DESENVOLVIMENTO ============
.PHONY: rebuild watch

# Recompilar tudo
rebuild: distclean all
	@echo "✓ Rebuild concluído"

# Watch mode (requer inotify-tools)
watch:
	@echo "👀 Monitorando ficheiros (Ctrl+C para parar)..."
	@while true; do \
		inotifywait -e modify $(SRC_DIR)/*.c $(INCLUDE_DIR)/*.h && make; \
	done

# ============ DEFAULT ============
.DEFAULT_GOAL := all
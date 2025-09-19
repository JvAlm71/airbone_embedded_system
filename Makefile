# Makefile para Sistema Embarcado de Aeronave
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -O2 -g
INCLUDES = -Iinclude
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Arquivos fonte
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/airborne_system

# Bibliotecas
LIBS = -lm -lpthread

.PHONY: all clean run debug

all: $(TARGET)

# Criar diretórios se não existirem
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Compilar objeto
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Linkar executável
$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) $(LIBS) -o $@
	@echo "Sistema embarcado compilado com sucesso!"

# Executar o programa
run: $(TARGET)
	./$(TARGET)

# Compilar com debug
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Limpar arquivos compilados
clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Arquivos de compilação removidos"

# Verificar dependências
check:
	@echo "Verificando dependências..."
	@which gcc || echo "ERRO: gcc não encontrado"
	@echo "Compilador: $$(gcc --version | head -1)"
	@echo "Pthread support: $$(echo '#include <pthread.h>' | gcc -E - > /dev/null 2>&1 && echo 'OK' || echo 'ERRO')"

# Instalar dependências (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y build-essential gcc libc6-dev

# Mostrar ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make          - Compilar o sistema"
	@echo "  make run      - Compilar e executar"
	@echo "  make debug    - Compilar com símbolos de debug"
	@echo "  make clean    - Limpar arquivos compilados"
	@echo "  make check    - Verificar dependências"
	@echo "  make help     - Mostrar esta ajuda"
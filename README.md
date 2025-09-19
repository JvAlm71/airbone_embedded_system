# Sistema Embarcado de Aeronave

Repositório para guardar as atualizações do sistema embarcado da disciplina.

## Descrição

Este projeto implementa um sistema embarcado multi-thread para monitoramento de aeronaves, utilizando pthreads e outras funcionalidades avançadas da linguagem C.

## Funcionalidades

- **Arquitetura Multi-thread**: Sistema baseado em pthreads para processamento concorrente
- **Simulação de Sensores**: Coleta de dados de temperatura, pressão e altitude
- **Buffer Circular Thread-Safe**: Comunicação segura entre threads usando mutexes e variáveis de condição
- **Processamento em Tempo Real**: Análise e processamento contínuo dos dados dos sensores
- **Monitoramento do Sistema**: Thread dedicada para monitorar o estado do sistema
- **Detecção de Condições Críticas**: Alertas automáticos para situações perigosas
- **Shutdown Graceful**: Parada segura do sistema com limpeza adequada dos recursos

## Estrutura do Projeto

```
.
├── src/                    # Código fonte
│   └── airborne_system.c   # Implementação principal
├── include/                # Cabeçalhos
│   └── airborne_system.h   # Definições e protótipos
├── config/                 # Configurações
│   └── system.conf         # Parâmetros do sistema
├── Makefile               # Sistema de build
└── README.md              # Documentação
```

## Compilação e Execução

### Pré-requisitos

- GCC (GNU Compiler Collection)
- Suporte a pthreads
- Sistema Unix/Linux

### Compilar

```bash
make
```

### Executar

```bash
make run
```

### Outras opções

```bash
make debug    # Compilar com símbolos de debug
make clean    # Limpar arquivos compilados
make check    # Verificar dependências
make help     # Mostrar ajuda
```

## Arquitetura

### Threads Implementadas

1. **Thread de Sensores**: Coleta dados dos sensores a intervalos regulares
2. **Thread de Processamento**: Analisa dados, calcula estatísticas e detecta condições críticas
3. **Thread de Monitoramento**: Monitora o estado geral do sistema

### Sincronização

- **Mutexes**: Proteção de recursos compartilhados
- **Variáveis de Condição**: Sincronização entre produtores e consumidores
- **Buffer Circular**: Armazenamento thread-safe de dados dos sensores

### Recursos C Utilizados

- Pthreads (POSIX Threads)
- Mutexes e Condition Variables
- Signal handling
- Estruturas e ponteiros
- Funções matemáticas
- Gerenciamento de memória
- I/O formatado

## Dados dos Sensores

O sistema simula os seguintes sensores:

- **Temperatura**: -50°C a 50°C (com alertas para valores críticos)
- **Pressão Atmosférica**: Variação baseada em altitude simulada
- **Altitude**: 0 a 10.000m (com alerta para altitudes excessivas)

## Parar o Sistema

Para parar o sistema graciosamente, pressione `Ctrl+C`. Isso ativará o signal handler que fará a parada segura de todas as threads.

## Contribuição

Este projeto faz parte de uma disciplina acadêmica. Contribuições devem seguir as diretrizes do curso.

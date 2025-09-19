/*
 * Cliente TCP - Jogo da Velha
 * Compilar: gcc -Wall -lpthread desafio3_client.c -o cliente_velha
 * Uso:      ./cliente_velha <host> <porta>
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FLOW_SIZE 1024

static int sockId = -1;
static char my_sym = '?';
static int my_turn = 0;
static pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

static void print_board(const char *s) {
    if (!s || strlen(s) < 9) return;
    printf("\nTabuleiro:\n");
    for (int i = 0; i < 9; i++) {
        char c = s[i];
        char d = (c == '.') ? ' ' : c;
        printf(" %c ", d);
        if (i % 3 != 2) printf("|");
        else if (i != 8) printf("\n---+---+---\n");
    }
    printf("\n\n");
    printf("Posicoes: 0 1 2 / 3 4 5 / 6 7 8\n");
}

static void send_line(const char *fmt, ...) {
    char out[MAX_FLOW_SIZE];
    va_list ap; va_start(ap, fmt);
    vsnprintf(out, sizeof(out), fmt, ap);
    va_end(ap);
    size_t len = strnlen(out, sizeof(out));
    if (len == 0 || out[len-1] != '\n') {
        if (len + 1 < sizeof(out)) { out[len] = '\n'; out[len+1] = '\0'; len++; }
    }
    send(sockId, out, len, 0);
}

static int recv_line(int fd, char *buf, size_t n, char *carry, size_t *carry_len) {
    size_t len = *carry_len;
    memcpy(buf, carry, len);
    while (1) {
        for (size_t i = 0; i < len; i++) {
            if (buf[i] == '\n') {
                buf[i] = '\0';
                size_t rest = len - (i + 1);
                memmove(carry, buf + i + 1, rest);
                *carry_len = rest;
                return 1;
            }
        }
        if (len + 1 >= n) { buf[n-1] = '\0'; *carry_len = 0; return 1; }
        ssize_t r = recv(fd, buf + len, n - 1 - len, 0);
        if (r <= 0) return 0;
        len += (size_t)r;
    }
}

static void *rx_thread(void *arg) {
    (void)arg;
    char line[MAX_FLOW_SIZE];
    char carry[MAX_FLOW_SIZE]; size_t carry_len = 0;

    while (1) {
        int ok = recv_line(sockId, line, sizeof(line), carry, &carry_len);
        if (!ok) {
            printf("Conexao encerrada.\n");
            exit(0);
        }
        if (strncmp(line, "ASSIGN ", 7) == 0) {
            pthread_mutex_lock(&mut);
            my_sym = line[7];
            pthread_mutex_unlock(&mut);
            printf("Voce eh o jogador %c\n", my_sym);
        } else if (strncmp(line, "WAITING ", 8) == 0) {
            printf("Aguardando outro jogador... (%s)\n", line + 8);
        } else if (strcmp(line, "START") == 0) {
            printf("Partida iniciada!\n");
        } else if (strncmp(line, "BOARD ", 6) == 0) {
            print_board(line + 6);
        } else if (strncmp(line, "TURN ", 5) == 0) {
            char t = line[5];
            pthread_mutex_lock(&mut);
            my_turn = (t == my_sym);
            pthread_mutex_unlock(&mut);
            if (my_turn) {
                printf("Sua vez (%c). Digite posicao [0-8] ou END para sair:\n", my_sym);
            } else {
                printf("Vez do oponente (%c)...\n", t);
            }
        } else if (strncmp(line, "OK MOVE ", 8) == 0) {
            // opcional: eco do servidor
        } else if (strncmp(line, "ERR ", 4) == 0) {
            printf("Erro: %s\n", line + 4);
        } else if (strncmp(line, "WIN ", 4) == 0) {
            char w = line[4];
            if (w == my_sym) printf("Voce venceu!\n");
            else printf("Voce perdeu.\n");
        } else if (strcmp(line, "DRAW") == 0) {
            printf("Empate.\n");
        } else if (strcmp(line, "OPP_LEFT") == 0) {
            printf("Oponente desconectou. Partida encerrada.\n");
        } else if (strcmp(line, "BYE") == 0) {
            printf("Servidor finalizou a partida.\n");
            exit(0);
        } else {
            // outras infos
            // printf("Srv: %s\n", line);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s <host> <porta>\n", argv[0]);
        return 1;
    }
    struct hostent *hp = gethostbyname(argv[1]);
    if (!hp) { printf("Host invalido: %s\n", argv[1]); return 1; }
    int port = atoi(argv[2]);
    if (port <= 0 || port > 65535) { printf("Porta invalida: %s\n", argv[2]); return 1; }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    sockId = socket(AF_INET, SOCK_STREAM, 0);
    if (sockId < 0) { perror("socket"); return 1; }

    if (connect(sockId, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect");
        close(sockId);
        return 1;
    }

    pthread_t th;
    pthread_create(&th, NULL, rx_thread, NULL);
    pthread_detach(th);

    // Loop de entrada do usuário
    char in[MAX_FLOW_SIZE];
    while (fgets(in, sizeof(in), stdin)) {
        // remove \n
        size_t len = strlen(in);
        if (len && in[len-1] == '\n') in[len-1] = '\0';
        if (strcasecmp(in, "END") == 0) {
            send_line("END");
            break;
        }
        // tenta interpretar como posicao
        char *end = NULL;
        long v = strtol(in, &end, 10);
        if (end != in && *end == '\0') {
            pthread_mutex_lock(&mut);
            int can = my_turn;
            pthread_mutex_unlock(&mut);
            if (!can) {
                printf("Nao eh sua vez.\n");
                continue;
            }
            if (v < 0 || v > 8) {
                printf("Posicao invalida. Use 0..8.\n");
                continue;
            }
            send_line("MOVE %ld", v);
        } else {
            printf("Comando invalido. Use [0-8] para jogar ou END para sair.\n");
        }
    }

    close(sockId);
    return 0;
}


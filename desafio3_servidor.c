/*
 * Servidor TCP - Jogo da Velha (2 jogadores)
 * Compilar: gcc -Wall -lpthread desafio3_servidor.c -o servidor_velha
 * Uso:      ./servidor_velha [porta]  (padrão: 5000)
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
#include <errno.h>

#define QUEUE_LENGTH 5
#define MAX_FLOW_SIZE 1024
#define DEFAULT_PORT 5000

typedef struct {
    int clients[2];      // fds dos clientes; -1 se vazio
    char symbols[2];     // 'X' ou 'O'
    int count;           // conectados
    int current;         // índice do jogador da vez (0 ou 1)
    char board[9];       // '.', 'X', 'O'
    int started;         // 1 quando dois conectados
    int game_over;       // 1 quando terminou
} game_t;

static game_t game;
static pthread_mutex_t gmut = PTHREAD_MUTEX_INITIALIZER;

static void send_line(int fd, const char *fmt, ...) {
    char out[MAX_FLOW_SIZE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, sizeof(out), fmt, ap);
    va_end(ap);
    size_t len = strnlen(out, sizeof(out));
    // garante \n no final
    if (len == 0 || out[len-1] != '\n') {
        if (len + 1 < sizeof(out)) {
            out[len] = '\n';
            out[len+1] = '\0';
            len++;
        }
    }
    send(fd, out, len, 0);
}

static void bcast(const char *fmt, ...) {
    char out[MAX_FLOW_SIZE];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(out, sizeof(out), fmt, ap);
    va_end(ap);
    size_t len = strnlen(out, sizeof(out));
    if (len == 0 || out[len-1] != '\n') {
        if (len + 1 < sizeof(out)) {
            out[len] = '\n'; out[len+1] = '\0'; len++;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (game.clients[i] != -1) send(game.clients[i], out, len, 0);
    }
}

static void board_to_str(char *buf, size_t n) {
    for (int i = 0; i < 9 && i < (int)(n-1); i++) buf[i] = game.board[i];
    buf[9 < (int)(n-1) ? 9 : (int)(n-1)] = '\0';
}

static int check_winner(char sym) {
    const int L[8][3] = {
        {0,1,2},{3,4,5},{6,7,8},  // linhas
        {0,3,6},{1,4,7},{2,5,8},  // colunas
        {0,4,8},{2,4,6}           // diagonais
    };
    for (int i = 0; i < 8; i++) {
        if (game.board[L[i][0]] == sym &&
            game.board[L[i][1]] == sym &&
            game.board[L[i][2]] == sym) return 1;
    }
    return 0;
}

static int board_full(void) {
    for (int i = 0; i < 9; i++) if (game.board[i] == '.') return 0;
    return 1;
}

static void start_game_if_ready(void) {
    if (game.count == 2 && !game.started) {
        game.started = 1;
        game.current = 0; // X começa
        char b[16]; board_to_str(b, sizeof(b));
        bcast("START");
        bcast("BOARD %s", b);
        bcast("TURN %c", game.symbols[game.current]);
    }
}

typedef struct {
    int slot; // 0 ou 1
} client_arg_t;

static int safe_parse_int(const char *s, int *out) {
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return -1;
    if (v < -2147483648L || v > 2147483647L) return -1;
    *out = (int)v;
    return 0;
}

static void handle_move_locked(int slot, int pos) {
    if (!game.started) { send_line(game.clients[slot], "ERR Partida ainda nao iniciou"); return; }
    if (game.game_over) { send_line(game.clients[slot], "ERR Partida encerrada"); return; }
    if (slot != game.current) { send_line(game.clients[slot], "ERR Nao eh sua vez"); return; }
    if (pos < 0 || pos > 8) { send_line(game.clients[slot], "ERR Posicao invalida"); return; }
    if (game.board[pos] != '.') { send_line(game.clients[slot], "ERR Casa ocupada"); return; }

    char sym = game.symbols[slot];
    game.board[pos] = sym;

    char b[16]; board_to_str(b, sizeof(b));
    bcast("OK MOVE %d", pos);
    bcast("BOARD %s", b);

    if (check_winner(sym)) {
        game.game_over = 1;
        bcast("WIN %c", sym);
        bcast("BYE");
        return;
    }
    if (board_full()) {
        game.game_over = 1;
        bcast("DRAW");
        bcast("BYE");
        return;
    }
    game.current = 1 - game.current;
    bcast("TURN %c", game.symbols[game.current]);
}

static int recv_line(int fd, char *buf, size_t n, char *carry, size_t *carry_len) {
    // Usa carry como buffer acumulado até '\n'
    size_t len = *carry_len;
    memcpy(buf, carry, len);
    while (1) {
        // procura '\n'
        for (size_t i = 0; i < len; i++) {
            if (buf[i] == '\n') {
                buf[i] = '\0';
                // move resto para carry
                size_t rest = len - (i + 1);
                memmove(carry, buf + i + 1, rest);
                *carry_len = rest;
                return 1;
            }
        }
        if (len + 1 >= n) { // linha muito longa
            buf[n-1] = '\0';
            *carry_len = 0;
            return 1;
        }
        ssize_t r = recv(fd, buf + len, n - 1 - len, 0);
        if (r <= 0) return 0; // desconectado/erro
        len += (size_t)r;
    }
}

static void *client_thread(void *arg) {
    client_arg_t info = *(client_arg_t *)arg;
    free(arg);
    int slot = info.slot;

    // Mensagens iniciais ao cliente
    pthread_mutex_lock(&gmut);
    char sym = game.symbols[slot];
    send_line(game.clients[slot], "ASSIGN %c", sym);
    send_line(game.clients[slot], "WAITING %d/2", game.count);
    if (game.started) {
        char b[16]; board_to_str(b, sizeof(b));
        send_line(game.clients[slot], "START");
        send_line(game.clients[slot], "BOARD %s", b);
        send_line(game.clients[slot], "TURN %c", game.symbols[game.current]);
    }
    pthread_mutex_unlock(&gmut);

    char line[MAX_FLOW_SIZE];
    char carry[MAX_FLOW_SIZE]; size_t carry_len = 0;

    while (1) {
        int ok = recv_line(game.clients[slot], line, sizeof(line), carry, &carry_len);
        if (!ok) {
            // desconectou
            pthread_mutex_lock(&gmut);
            int fd = game.clients[slot];
            game.clients[slot] = -1;
            if (!game.game_over) {
                game.game_over = 1;
                bcast("OPP_LEFT");
                bcast("BYE");
            }
            pthread_mutex_unlock(&gmut);
            if (fd != -1) close(fd);
            break;
        }
        // parse comando
        if (strncmp(line, "MOVE ", 5) == 0) {
            int pos;
            if (safe_parse_int(line + 5, &pos) == 0) {
                pthread_mutex_lock(&gmut);
                handle_move_locked(slot, pos);
                pthread_mutex_unlock(&gmut);
            } else {
                pthread_mutex_lock(&gmut);
                send_line(game.clients[slot], "ERR Comando invalido");
                pthread_mutex_unlock(&gmut);
            }
        } else if (strcmp(line, "END") == 0) {
            pthread_mutex_lock(&gmut);
            int fd = game.clients[slot];
            game.clients[slot] = -1;
            if (!game.game_over) {
                game.game_over = 1;
                bcast("OPP_LEFT");
                bcast("BYE");
            }
            pthread_mutex_unlock(&gmut);
            if (fd != -1) close(fd);
            break;
        } else {
            pthread_mutex_lock(&gmut);
            send_line(game.clients[slot], "ERR Comando desconhecido");
            pthread_mutex_unlock(&gmut);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // Inicializa estado do jogo
    memset(&game, 0, sizeof(game));
    game.clients[0] = game.clients[1] = -1;
    game.symbols[0] = 'X'; game.symbols[1] = 'O';
    for (int i = 0; i < 9; i++) game.board[i] = '.';
    game.count = 0; game.current = 0; game.started = 0; game.game_over = 0;

    int port = DEFAULT_PORT;
    if (argc == 2) {
        int p = atoi(argv[1]);
        if (p > 0 && p <= 65535) port = p;
    }

    int sockId = socket(AF_INET, SOCK_STREAM, 0);
    if (sockId < 0) { perror("socket"); return 1; }

    int yes = 1;
    setsockopt(sockId, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

    if (bind(sockId, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind");
        close(sockId);
        return 1;
    }

    socklen_t slen = sizeof(server);
    if (getsockname(sockId, (struct sockaddr *)&server, &slen) == 0) {
        printf("Servidor na porta: %d\n", ntohs(server.sin_port));
    }

    if (listen(sockId, QUEUE_LENGTH) < 0) {
        perror("listen");
        close(sockId);
        return 1;
    }

    while (1) {
        struct sockaddr_in client;
        socklen_t clen = sizeof(client);
        int conn = accept(sockId, (struct sockaddr *)&client, &clen);
        if (conn < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }

        pthread_mutex_lock(&gmut);
        if (game.count >= 2 || game.game_over) {
            send_line(conn, "ERR Sala cheia");
            send_line(conn, "BYE");
            close(conn);
            pthread_mutex_unlock(&gmut);
            continue;
        }
        int slot = (game.clients[0] == -1) ? 0 : 1;
        game.clients[slot] = conn;
        game.count++;
        // Se ambos conectados, inicia
        start_game_if_ready();

        client_arg_t *arg = (client_arg_t *)malloc(sizeof(client_arg_t));
        arg->slot = slot;
        pthread_t th;
        pthread_create(&th, NULL, client_thread, arg);
        pthread_detach(th);
        pthread_mutex_unlock(&gmut);
    }

    close(sockId);
    return 0;
}


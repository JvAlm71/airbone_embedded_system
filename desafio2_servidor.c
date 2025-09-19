/*
 * servidorMonoUDP.c
 *
 * Mantém o estado (posX|posY|tam) e distribui atualizações para todos os clientes que enviam mensagens.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SIZE 500
#define SERVER_PORT 4567
#define MAX_CLIENTS 100

int main(int argc, char *argv[]) {
    int sockId, recvBytes;
    socklen_t addrLen;
    struct sockaddr_in server, clientAddr;
    char buf[SIZE];

    // estado do objeto
    int posX = 0, posY = 0, tam = 0;

    // lista simples de clientes
    struct sockaddr_in clients[MAX_CLIENTS];
    int clientCount = 0;

    if ((sockId = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("Datagram socket nao pode ser aberto\n");
       return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    if (bind(sockId, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("O bind para o datagram socket falhou\n");
        close(sockId);
        return 1;
    }

    printf("Servidor UDP rodando na porta %d\n", SERVER_PORT);
    addrLen = sizeof(clientAddr);

    while (1) {
        // recebe de qualquer cliente
        recvBytes = recvfrom(sockId, buf, SIZE-1, 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (recvBytes < 0) {
            perror("recvfrom");
            continue;
        }
        buf[recvBytes] = '\0';

        // registra cliente se novo (compara IP e porta)
        int found = 0;
        for (int i = 0; i < clientCount; ++i) {
            if (clients[i].sin_addr.s_addr == clientAddr.sin_addr.s_addr &&
                clients[i].sin_port == clientAddr.sin_port) {
                found = 1;
                break;
            }
        }   
        if (!found && clientCount < MAX_CLIENTS) {
            clients[clientCount++] = clientAddr;
            printf("Novo cliente registrado: %s:%d\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
        }

        // tenta parsear mensagem posX|posY|tam
        int nx, ny, nt;
        if (sscanf(buf, "%d|%d|%d", &nx, &ny, &nt) == 3) {
            posX = nx; posY = ny; tam = nt;
            printf("Atualizacao recebida de %s:%d -> %d|%d|%d\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port),
                    posX, posY, tam);
        } else {
            printf("Mensagem invalida de %s:%d -> %s\n",
                   inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buf);
            // NÃO prossegue em broadcast se inválida; continue;
        }

        // prepara mensagem de estado atual e envia para todos os clientes conhecidos
        char out[SIZE];
        int len = snprintf(out, sizeof(out), "%d|%d|%d", posX, posY, tam);
        for (int i = 0; i < clientCount; ++i) {
            if (sendto(sockId, out, len+1, 0, (struct sockaddr *)&clients[i], sizeof(clients[i])) < 0) {
                perror("sendto");
            }
        }
    }

    close(sockId);
    return 0;
}
/*
 * clienteMonoUDP.c
 *
 * Envia atualizações posX|posY|tam para o servidor e recebe broadcasts de estado.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

#define SIZE 500
#define SERVER_PORT 4567

int main(int argc, char *argv[]) {
    int sockId;
    struct sockaddr_in clientAddr, serverAddr;
    char buf[SIZE];
    struct hostent *hp;
    socklen_t servLen = sizeof(serverAddr);

    if (argc < 2) {
       printf("Uso: %s Endereco_do_servidor\n", argv[0]);
       return 1;
    }

    if ((sockId = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("Datagram socket nao pode ser aberto\n");
       return 1;
    }

    // permitir envio para endereços de broadcast
    int broadcast = 1;
    if (setsockopt(sockId, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("setsockopt SO_BROADCAST");
        close(sockId);
        return 1;
    }

    hp = gethostbyname(argv[1]);
    if (!hp) {
        printf("Host Invalido: %s\n", argv[1]);
        close(sockId);
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    memcpy((char*)&serverAddr.sin_addr, (char*)hp->h_addr, hp->h_length);
    serverAddr.sin_port = htons(SERVER_PORT);

    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(0);

    if (bind(sockId, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
        printf("O bind para o datagram socket falhou\n");
        close(sockId);
        return 1;
    }

    printf("Digite mensagens no formato posX|posY|tam (ex: 50|77|20). 'exit' para sair.\n");
    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(buf, SIZE, stdin)) break;
        // remove newline
        buf[strcspn(buf, "\n")] = 0;
        if (strcmp(buf, "exit") == 0) break;
        if (strlen(buf) == 0) continue;

        // envia ao servidor
        if (sendto(sockId, buf, strlen(buf)+1, 0, (struct sockaddr *)&serverAddr, servLen) < 0) {
            perror("sendto");
            continue;
        }

        // aguarda resposta (estado atual do servidor)
        ssize_t r = recvfrom(sockId, buf, SIZE-1, 0, (struct sockaddr *)&serverAddr, &servLen);
        if (r < 0) {
            perror("recvfrom");
            continue;
        }
        buf[r] = '\0';
        printf("Estado atual: %s\n", buf);
    }

    close(sockId);
    return 0;
}


/*
 * 			servidorMonoUDP.c
 *
 * Este programa servidor foi desenvolvido para receber mensagens de uma aplicacao cliente UDP
 *
 * Funcao:     Enviar e receber mensagens compostas de caracteres
 * Plataforma: Linux (Unix), ou Windows com CygWin
 * Compilar:   gcc -Wall servidorMonoUDP.c -o servidorMonoUDP
 * Uso:        ./servidorMonoUDP
 *
 * Autor:      Jose Martins Junior
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>

#define SIZE 300            //Tamanho maximo do buffer de caracteres
#define SERVER_PORT 4567    //Porta do servidor
#define true 1

int main(int argc, char *argv[]) {
    int sockId, recvBytes;
    struct sockaddr_in server;
    char buf[SIZE];

    if ((sockId = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("Datagram socket nao pode ser aberto\n");
       return(1);
    }

    // Permite reuso rápido da porta ao reiniciar o servidor
    int yes = 1;
    setsockopt(sockId, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    if (bind(sockId, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("O bind para o datagram socket falhou\n");
        close(sockId);
        return(1);
    }

    while(true) {
        struct sockaddr_in from;
        socklen_t fromLen = sizeof(from);
        memset(&from, 0, sizeof(from));
        memset(buf, 0, sizeof(buf));

        recvBytes = recvfrom(sockId, buf, SIZE - 1, 0, (struct sockaddr *)&from, &fromLen);
        if (recvBytes <= 0) continue;
        buf[recvBytes] = '\0'; // garante string terminada

        // Parse "T|U"
        char *sep = strchr(buf, '|');
        if (sep) {
            *sep = '\0';
            const char *t = buf;
            const char *u = sep + 1;
            printf("Temperatura: %s C, Umidade: %s %%\n", t, u);
        } else {
            // Caso mensagem fora do formato, mostra bruta
            printf("Mensagem bruta: %s\n", buf);
        }

        // Opcional: eco/ACK (desnecessário para broadcast)
        // sendto(sockId, "OK", 2, 0, (struct sockaddr *)&from, fromLen);
    }
    close(sockId);
    return(0);
}


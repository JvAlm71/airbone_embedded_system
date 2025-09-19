/*
 * 			clienteMonoUDP.c
 *
 * Este programa cliente foi desenvolvido para enviar mensagens para uma aplicacao servidora UDP
 *
 * Funcao:     Enviar e receber mensagens compostas de caracteres
 * Plataforma: Linux (Unix), ou Windows com CygWin
 * Compilar:   gcc -Wall clienteMonoUDP.c -o clienteMonoUDP
 * Uso:        ./clienteMonoUDP Endereco_do_servidor_ou_broadcast
 *
 * Autor:      Jose Martins Junior
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>

#define SIZE 300            //Tamanho maximo do buffer de caracteres
#define SERVER_PORT 4567    //Porta do servidor
#define true 1

// Função utilitária para gerar float no intervalo [min, max]
static float randf(float min, float max) {
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

int main(int argc, char *argv[]) {
    int sockId, servLen;
    struct sockaddr_in client, server;
    char buf[SIZE];
    struct hostent *hp;

/*
 *******************************************************************************
               Abrindo um socket do tipo datagram (UDP)
 *******************************************************************************
 */
    if ((sockId = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       printf("Datagram socket nao pode ser aberto\n");
       return(1);
    }

    // Habilita broadcast
    int on = 1;
    if(setsockopt(sockId, SOL_SOCKET, SO_BROADCAST, (char *)&on, sizeof(on)) < 0) {
       printf("Opcao de broadcast nao pode ser atribuida ao datagram socket\n");
       close(sockId);
       return(1);
    }

    // Zera estruturas
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));
    memset(buf, 0, sizeof(buf));

    // Destino: broadcast se não houver argumento, senão resolve o host/IP passado
    if (argc < 2) {
       server.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    } else {
       hp = gethostbyname(argv[1]);
       if (((char *) hp) == NULL) {
           printf("Host Invalido: %s\n", argv[1]);
           return(1);
       } else {
           memcpy((char*)&server.sin_addr, (char*)hp->h_addr, hp->h_length);
       }
    }
    server.sin_port = htons(SERVER_PORT);
    server.sin_family = AF_INET;

    // Fonte: porta efêmera em qualquer interface
    client.sin_family = AF_INET;
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    client.sin_port = htons(0);

    if (bind(sockId, (struct sockaddr *)&client, sizeof(client)) < 0) {
        printf("O bind para o datagram socket falhou\n");
        close(sockId);
        return(1);
    }

    srand((unsigned)time(NULL));

    while(true){
        char key = getchar();
        if(key == 'Q' || key == 'q'){
            printf("encerra saporra");
            break;
        }else if(key == 'X' || key == 'x'){
    float temperatura = randf(20.0f, 35.0f);
    float umidade = randf(30.0f, 80.0f);
    int len = snprintf(buf, sizeof(buf), "%.2f|%.2f", temperatura, umidade);

    servLen = sizeof(server);
    if (sendto(sockId, buf, len, 0, (struct sockaddr *)&server, servLen) < 0) {
        perror("sendto");
        close(sockId);
        return 1;
    }
    
    printf("Enviado: %s\n", buf);

}
}


/*----------------------------------------------------------------------------*/

/*
 *******************************************************************************
               Enviando e recebendo as mensagens do servidor
 *******************************************************************************
 */
    // printf("Digite a mensagem: ");
    // scanf("%s",buf);
    // servLen = sizeof(server);
    // sendto(sockId, buf, sizeof(buf), 0, (struct sockaddr *)&server, servLen);
//    recvfrom(sockId, buf, SIZE, 0, (struct sockaddr *)&server, &servLen);
//    printf("Mensagem retornada: %s\n", buf);

/*----------------------------------------------------------------------------*/
    close(sockId);
    return(0);
}


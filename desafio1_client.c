#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define BROADCAST_IP "255.255.255.255" // Ou o broadcast específico da sua rede
#define PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in broadcast_addr;
    char buffer[BUFFER_SIZE];
    
    // 1. Criar socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Falha ao criar socket");
        exit(EXIT_FAILURE);
    }

    // 2. Habilitar a opção de broadcast no socket
    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Falha ao configurar setsockopt para broadcast");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    
    // 3. Configurar o endereço de destino (broadcast)
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    srand(time(NULL)); // Inicializa gerador de números aleatórios

    while(1) {
        // Simular dados dos sensores
        float temperatura = 20.0 + (rand() % 150) / 10.0; // Temp entre 20.0 e 35.0
        float umidade = 50.0 + (rand() % 300) / 10.0;     // Umidade entre 50.0 e 80.0

        // Formatar a mensagem conforme especificado
        snprintf(buffer, BUFFER_SIZE, "%.2f|%.2f", temperatura, umidade);
        
        // 4. Enviar a mensagem para o endereço de broadcast
        sendto(sockfd, buffer, strlen(buffer), 0, (const struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
        
        printf("Enviado: %s\n", buffer);
        
        sleep(5); // Envia a cada 5 segundos
    }

    close(sockfd);
    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define SERVER_PORT 8888

int client_socket;

void* receive_messages(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0) {
            printf("\nDisconnected from server\n");
            exit(EXIT_SUCCESS);
        }
        buffer[received] = '\0';
        printf("%s", buffer);
    }
    return NULL;
}

void handle_signal(int sig) {
    if (sig == SIGINT) {
        printf("\nDisconnecting...\n");
        close(client_socket);
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char* argv[]) {
    struct sockaddr_in server_addr;
    pthread_t receive_thread;
    
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // 시그널 핸들러 설정
    signal(SIGINT, handle_signal);
    
    // 소켓 생성
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(SERVER_PORT);
    
    // 서버에 연결
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server. Type your messages (Ctrl+C to quit):\n");
    
    // 메시지 수신 스레드 생성
    if (pthread_create(&receive_thread, NULL, receive_messages, NULL) != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 메인 스레드에서 메시지 송신
    char buffer[BUFFER_SIZE];
    while (1) {
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
                perror("Send failed");
                break;
            }
        }
    }
    
    close(client_socket);
    return 0;
}
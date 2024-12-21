#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_EVENTS 32
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100
#define PORT 8888

typedef struct {
    int socket;
    char nickname[32];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

void add_client(int client_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            sprintf(clients[i].nickname, "User%d", i);
            client_count++;
            
            // 입장 메시지 준비
            char welcome_msg[BUFFER_SIZE];
            sprintf(welcome_msg, "Welcome %s! Total users: %d\n", clients[i].nickname, client_count);
            send(client_socket, welcome_msg, strlen(welcome_msg), 0);
            
            printf("New client connected: %s (Total: %d)\n", clients[i].nickname, client_count);
            break;
        }
    }
}

void remove_client(int client_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket) {
            printf("Client disconnected: %s (Total: %d)\n", clients[i].nickname, client_count - 1);
            clients[i].socket = 0;
            memset(clients[i].nickname, 0, sizeof(clients[i].nickname));
            client_count--;
            break;
        }
    }
}

void broadcast_message(int sender_socket, const char* message) {
    char sender_nickname[32] = "";
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sender_socket) {
            strcpy(sender_nickname, clients[i].nickname);
            break;
        }
    }
    
    char formatted_message[BUFFER_SIZE];
    sprintf(formatted_message, "%s: %s", sender_nickname, message);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) {
            send(clients[i].socket, formatted_message, strlen(formatted_message), 0);
        }
    }
}

void set_nonblocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    
    // 서버 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    
    // SO_REUSEADDR 옵션 설정
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 바인딩
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }
    
    // 리스닝
    if (listen(server_socket, 5) == -1) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    // epoll 인스턴스 생성
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("Epoll creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 서버 소켓을 epoll에 등록
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        perror("Epoll control failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Chat server started on port %d\n", PORT);
    
    struct epoll_event events[MAX_EVENTS];
    while (1) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_socket) {
                // 새 클라이언트 연결 처리
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_size);
                
                if (client_socket == -1) {
                    if (errno != EAGAIN) {
                        perror("Accept failed");
                    }
                    continue;
                }
                
                set_nonblocking(client_socket);
                
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    perror("Epoll control failed");
                    close(client_socket);
                    continue;
                }
                
                add_client(client_socket);
            } else {
                // 클라이언트로부터 데이터 수신
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                ssize_t received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (received <= 0) {
                    if (received == 0 || errno != EAGAIN) {
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        remove_client(client_fd);
                        close(client_fd);
                    }
                } else {
                    buffer[received] = '\0';
                    broadcast_message(client_fd, buffer);
                }
            }
        }
    }
    
    close(server_socket);
    close(epoll_fd);
    return 0;
}
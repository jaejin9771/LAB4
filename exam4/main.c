#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define NUM_CLIENTS 3  // 클라이언트 쓰레드 수
#define BUFFER_SIZE 128
// 공유 자원
char buffer[BUFFER_SIZE];         // 메시지 버퍼
int ready_flag =0;               // 메시지가 준비되었는지 여부

// 동기화를 위한 뮤텍스와 조건 변수
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_message = PTHREAD_COND_INITIALIZER;  // 메시지 작성 알림
pthread_cond_t cond_broadcast = PTHREAD_COND_INITIALIZER;  // 방송 알림

// 클라이언트 쓰레드 함수
void *client_thread(void *arg) {
    int id =*((int *)arg);
    while (1) {
        // 서버에 메시지 전송 요청
        pthread_mutex_lock(&mutex);
        snprintf(buffer, BUFFER_SIZE, "Message from Client %d", id);
        ready_flag =1;
        // 메시지가 준비되었음을 서버에 알림
        pthread_cond_signal(&cond_message);
        pthread_mutex_unlock(&mutex);
        // 서버로부터 메시지 수신 대기
        pthread_mutex_lock(&mutex);
        while (ready_flag !=0) {
            pthread_cond_wait(&cond_broadcast, &mutex);
        }
        printf("Client %d received: %s\n", id, buffer);
        pthread_mutex_unlock(&mutex);
        sleep(rand() % 3 +1);  // 랜덤한 시간 대기
    }
    pthread_exit(NULL);
}

// 서버 쓰레드 함수
void *server_thread(void *arg) {
    while (1) {
        // 클라이언트 메시지 대기
        pthread_mutex_lock(&mutex);
        while (ready_flag ==0) {
            pthread_cond_wait(&cond_message, &mutex);
        }
        // 메시지 방송
        printf("Server broadcasting: %s\n", buffer);
        ready_flag =0;
        // 모든 클라이언트에게 방송 알림
        pthread_cond_broadcast(&cond_broadcast);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t clients[NUM_CLIENTS];  // 클라이언트 쓰레드
    pthread_t server;                // 서버 쓰레드
    int client_ids[NUM_CLIENTS];
    int status;
    // 서버 쓰레드 생성
    status = pthread_create(&server, NULL, server_thread, NULL);
    if (status !=0) {
        perror("Failed to create server thread");
        exit(1);
    }
    // 클라이언트 쓰레드 생성
    for (int i =0; i < NUM_CLIENTS; i++) {
        client_ids[i] = i +1;
        status = pthread_create(&clients[i], NULL, client_thread, &client_ids[i]);
        if (status !=0) {
            perror("Failed to create client thread");
            exit(1);
        }
    }
    // 쓰레드 종료 대기
    pthread_join(server, NULL);
    for (int i =0; i < NUM_CLIENTS; i++) {
        pthread_join(clients[i], NULL);
    }
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5    // 버퍼 크기
#define NUM_PRODUCERS 2  // 생산자 쓰레드 수
#define NUM_CONSUMERS 2  // 소비자 쓰레드 수

// 공유 자원: 버퍼와 관련 변수
int buffer[BUFFER_SIZE];  // 고정 크기 버퍼
int count = 0;            // 버퍼 내 항목 수
int in = 0;               // 버퍼에 추가할 위치
int out = 0;              // 버퍼에서 제거할 위치

// 동기화를 위한 뮤텍스와 조건 변수
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_empty = PTHREAD_COND_INITIALIZER;

// 생산자 함수
void *producer(void *arg) {
    int id = *((int *)arg);
    while (1) {
        int item = rand() % 100;  // 랜덤 값 생성 (생산된 데이터)

        // 뮤텍스 잠금
        pthread_mutex_lock(&mutex);

        // 버퍼가 가득 찬 경우 대기
        while (count == BUFFER_SIZE) {
            printf("Producer %d: Buffer full, waiting...\n", id);
            pthread_cond_wait(&cond_empty, &mutex);
        }

        // 데이터 추가
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("Producer %d: Produced %d, Buffer count: %d\n", id, item, count);

        // 소비자 대기 해제
        pthread_cond_signal(&cond_full);

        // 뮤텍스 해제
        pthread_mutex_unlock(&mutex);

        sleep(rand() % 2 + 1);  // 랜덤한 시간 대기
    }
    pthread_exit(NULL);
}

// 소비자 함수
void *consumer(void *arg) {
    int id = *((int *)arg);
    while (1) {
        int item;

        // 뮤텍스 잠금
        pthread_mutex_lock(&mutex);

        // 버퍼가 비어 있는 경우 대기
        while (count == 0) {
            printf("Consumer %d: Buffer empty, waiting...\n", id);
            pthread_cond_wait(&cond_full, &mutex);
        }

        // 데이터 제거
        item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        printf("\tConsumer %d: Consumed %d, Buffer count: %d\n", id, item, count);

        // 생산자 대기 해제
        pthread_cond_signal(&cond_empty);

        // 뮤텍스 해제
        pthread_mutex_unlock(&mutex);

        sleep(rand() % 2 + 1);  // 랜덤한 시간 대기
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];
    int consumer_ids[NUM_CONSUMERS];

    // 생산자 쓰레드 생성
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        if (pthread_create(&producers[i], NULL, producer, &producer_ids[i]) != 0) {
            perror("Failed to create producer thread");
            exit(1);
        }
        printf("produce thread %d\n", i);
    }

    // 소비자 쓰레드 생성
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;
        if (pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]) != 0) {
            perror("Failed to create consumer thread");
            exit(1);
        }
        printf("consume thread %d\n", i);
    }

    // 쓰레드 종료 대기
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    return 0;
}

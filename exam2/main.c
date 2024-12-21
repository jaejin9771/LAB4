#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#define NUM_THREADS 5  // 생성할 쓰레드 개수
pthread_mutex_t mutex; // 뮤텍스 선언
int sum = 0;           // 공유 자원 (합계를 저장)
void *thread_function(void *arg)
{
    int thread_id = *((int *)arg);
    // 뮤텍스 잠금
    pthread_mutex_lock(&mutex);
    // 공유 자원 접근 (합산 작업)
    printf("Thread %d: Adding 10 to sum.\n", thread_id);
    sum += 10;
    // 뮤텍스 해제
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL); // 쓰레드 종료
}
int main()
{
    pthread_t threads[NUM_THREADS]; // 쓰레드 ID 배열
    int thread_ids[NUM_THREADS];    // 쓰레드 ID 값 저장
    int status;
    // 뮤텍스 초기화
    pthread_mutex_init(&mutex, NULL);
    // 쓰레드 생성
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_ids[i] = i + 1; // 쓰레드 ID 설정
        status = pthread_create(&threads[i], NULL, thread_function,
                                (void *)&thread_ids[i]);
        if (status != 0)
        {
            fprintf(stderr, "Error creating thread %d\n", i + 1);
            exit(1);
        }
    }
    // 쓰레드 종료 대기
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
        printf("join : thread %d \n", i+1);
    }
    // 결과 출력
    printf("Final sum: %d\n", sum);
    // 뮤텍스 해제
    pthread_mutex_destroy(&mutex);
    return 0;
}
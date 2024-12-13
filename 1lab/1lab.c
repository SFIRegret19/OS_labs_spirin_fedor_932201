#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int ready = 0;

void* producer(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);

        if (ready == 1) {
            pthread_mutex_unlock(&lock);
            continue;
        }

        ready = 1;
        printf("Producer: Event provided\n");

        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock);
        sleep(1);
    }
    return NULL;
}

void* consumer(void* arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        while (ready == 0) {
            pthread_cond_wait(&cond1, &lock);
        }

        printf("Consumer: Event consumed\n");
        ready = 0;

        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main() {
    pthread_t producer_thread, consumer_thread;

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond1);

    return 0;
}
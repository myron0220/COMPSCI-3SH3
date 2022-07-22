//
//  Macid: li64; Avenue name: Henry Li
//  Macid: wangm235; Avenue name: Mingzhe Wang
//
//

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define NUM_PHIL 5


enum {THINKING, HUNGRY, EATING} state[NUM_PHIL];
pthread_cond_t condition[NUM_PHIL];
pthread_mutex_t mutex;
int chopsticks[NUM_PHIL]; // 1: chopstick at table; 0: chopstick being used

void show_chopsticks() {
    for (int i = 0; i < NUM_PHIL; i++) {
        printf("%d", chopsticks[i]);
    }
    printf("\n");
}

void test(int i) {
    if ((state[(i + NUM_PHIL - 1) % NUM_PHIL] != EATING) &&
        (state[i] == HUNGRY) &&
        (state[(i + 1) % NUM_PHIL] != EATING)) {
        state[i] = EATING;
        pthread_cond_signal(&condition[i]);
    }
}

void pickup_forks(int i) {
    state[i] = HUNGRY;
    test(i);
    pthread_mutex_lock(&mutex);
    if (state[i] != EATING)
        pthread_cond_wait(&condition[i],&mutex);
    int left = i;
    int right = (i + 1) % NUM_PHIL;
    chopsticks[left] = 0;
    chopsticks[right] = 0;
    printf("Philosopher %d picks forks %d and %d\n", i, left, right);
    show_chopsticks();
    pthread_mutex_unlock(&mutex);
}

void return_forks(int i) {
    pthread_mutex_lock(&mutex);
    state[i] = THINKING;
    test((i + NUM_PHIL - 1) % NUM_PHIL);
    test((i + 1) % NUM_PHIL);
    int left = i;
    int right = (i + 1) % NUM_PHIL;
    chopsticks[left] = 1;
    chopsticks[right] = 1;
    printf("Philosopher %d returns forks %d and %d\n", i, left, right);
    show_chopsticks();
    pthread_mutex_unlock(&mutex);
}


void *runner(void *params){

    int id =  *((int *)params);
    int count = 0;

    while (count < 2) { // Leave the table after eating twice.
        int action_time = rand()%3 + 1;
        printf("Philosopher %d is thinking for %d seconds\n", id, action_time);
        sleep(action_time);
        pickup_forks(id);
        action_time = rand()%3 + 1;
        printf("Philosopher %d is eating for %d seconds\n", id, action_time);
        sleep(action_time);
        return_forks(id);
        count++;
    }

    pthread_exit(0);

}

int main() {
    pthread_t tid[NUM_PHIL];
    int param[NUM_PHIL];

    // Initialization
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < NUM_PHIL; i++) {
        chopsticks[i] = 1;
        state[i] = THINKING;
        param[i] = i;
        pthread_cond_init(&condition[i], NULL);
    }
    srand(time(0)); // use the current time as seed for random generator.

    // Dining Phils
    for (int i = 0; i < NUM_PHIL; i++) {
        pthread_create(&tid[i], NULL, runner, &param[i]);
    }

    for (int i = 0; i < NUM_PHIL; i++) {
        pthread_join(tid[i], NULL);
    }

    return 0;
}

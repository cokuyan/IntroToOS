#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 5
#define NUM_LOOPS 6

int resources = 0;
int waiters = 0;
int shared = 0;

pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c_read = PTHREAD_COND_INITIALIZER;
pthread_cond_t c_write = PTHREAD_COND_INITIALIZER;

void *reader(void *param);
void *writer(void *param);

int main(int argc, char *argv[]) {

  pthread_t readers[NUM_THREADS], writers[NUM_THREADS];
  int thread_ids[NUM_THREADS];

  int i;
  for (i = 0; i < NUM_THREADS; i++) {
    thread_ids[i] = i;
    if (pthread_create(&readers[i], NULL, reader, &thread_ids[i]) != 0) {
      fprintf(stderr, "Unable to create reader thread %d\n", i);
      exit(1);
    }
    if (pthread_create(&writers[i], NULL, writer, &thread_ids[i]) != 0) {
      fprintf(stderr, "Unable to create writer thread %d\n", i);
      exit(1);
    }
  }

  for (i = 0; i < NUM_THREADS; i++) {
    pthread_join(readers[i], NULL);
    pthread_join(writers[i], NULL);
  }

  printf("Completed processing %d readers and writers\n", NUM_THREADS);
  fflush(stdout);

  return 0;
}

void *reader(void *param) {
  int thread_id = *((int*) param);

  srand(thread_id + 1);
  int sleep_secs = rand() % 5;

  int i;
  for (i = 0; i < NUM_LOOPS; i++) {
    sleep(sleep_secs);
    pthread_mutex_lock(&counter_mutex);
      if (resources == -1) {
        waiters++;
        while (resources == -1)
          pthread_cond_wait(&c_read, &counter_mutex);
        waiters--;
      }
      resources++;
    pthread_mutex_unlock(&counter_mutex);

    printf("Reader thread %d: value read is %d\n", thread_id, shared);
    printf("Reader thread %d: readers present: %d\n", thread_id, resources);
    fflush(stdout);

    pthread_mutex_lock(&counter_mutex);
      resources--;
      if (resources == 0)
        pthread_cond_broadcast(&c_write);
      else
        pthread_cond_broadcast(&c_read);
    pthread_mutex_unlock(&counter_mutex);
  }

  printf("Reader thread %d: finished processing\n", thread_id);
  fflush(stdout);
  return 0;
}

void *writer(void *param) {
  int thread_id = *((int*) param);

  srand((thread_id + 1) * 2);
  int sleep_secs = rand() % 5;

  int i;
  for (i = 0; i < NUM_LOOPS; i++) {
    sleep(sleep_secs);
    pthread_mutex_lock(&counter_mutex);
      while (resources != 0)
        pthread_cond_wait(&c_write, &counter_mutex);
      resources = -1;
    pthread_mutex_unlock(&counter_mutex);

    shared = thread_id;
    printf("Writer thread %d: wrote value %d\n", thread_id, shared);
    if (resources == -1)
      printf("Writer thread %d: readers present: 0\n", thread_id);
    else
      printf("Writer thread %d: readers present: %d\n", thread_id, resources);
    fflush(stdout);

    pthread_mutex_lock(&counter_mutex);
      resources = 0;
      if (waiters > 0)
        pthread_cond_broadcast(&c_read);
      else
        pthread_cond_broadcast(&c_write);
    pthread_mutex_unlock(&counter_mutex);
  }

  printf("Writer thread %d: finished processing\n", thread_id);
  fflush(stdout);
  return 0;
}

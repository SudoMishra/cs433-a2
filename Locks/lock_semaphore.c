#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <semaphore.h>

#define N (10000000)
int num_threads;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int x = 0, y = 0;
sem_t semaphore;

void *critical(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        // Acquire (&lock);
        sem_wait(&semaphore);
	    {
            assert (x == y);
            x = y + 1;
            y++;
        }
        sem_post(&semaphore);
        // Release (&lock);
    }
}

int main(int argc, char* argv[])
{
    int i, *id;
    pthread_t *tid;
    pthread_attr_t attr;
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
    if(argc != 2)
    {
        printf("Need no. of threads\n");
        exit(1);
    }
    num_threads = atoi(argv[1]);
    tid = (pthread_t*)malloc(num_threads*sizeof(pthread_t));
	// private_sum = (double*)malloc(num_threads*sizeof(double));
	id = (int*)malloc(num_threads*sizeof(int));
 	for (i=0; i<num_threads; i++) 
        id[i] = i;
    sem_init(&semaphore, 0, 1);

    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<num_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &id[i]);
   	}
    for (i=0; i<num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    assert (x == y);
    assert (x == N*num_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}
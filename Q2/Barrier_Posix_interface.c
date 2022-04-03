#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (1000000)

int n_threads;

pthread_barrier_t pbar;

void *critical(void *param)
{
    int i;
    pthread_barrier_t *pbar = (pthread_barrier_t*)param;
    for (i=0; i<N; i++) 
    {
        pthread_barrier_wait(pbar);

        // BARRIER(bar);
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
    n_threads = atoi(argv[1]);
    id = (int*)malloc(n_threads*sizeof(int));
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
    for (i=0; i<n_threads; i++) 
        id[i] = i;
    // barrier bar;
    // bar_init(&bar);
    pthread_barrier_init(&pbar,NULL,n_threads);
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &pbar);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}

    gettimeofday(&tv1, &tz1);
    
    // assert(x==y);
    // assert(x==N*n_threads);
    printf("time: %ld microseconds\n",(tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    // tid = (pthread_t*)malloc(num_threads*sizeof(pthread_t));

    return 0;
}
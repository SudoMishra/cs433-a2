#include <stdio.h>
#include <stdlib.h>
// #include <pthread.h>
#include <omp.h>
#include <sys/time.h>
#include <assert.h>

#define N (1000000)

int n_threads;


void critical(void* param)
{
    int i;
    for (i=0; i<N; i++) {
        #pragma omp barrier
    }
}


int main(int argc, char* argv[])
{
    // pthread_t *tid;
    // pthread_attr_t attr;
    int i;
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
    if(argc != 2)
    {
        printf("Need no. of threads\n");
        exit(1);
    }
    n_threads = atoi(argv[1]);

    gettimeofday(&tv0, &tz0);
    #pragma omp parallel num_threads (n_threads) private (i)
    {
        for (i=0; i<N; i++) 
        {
            #pragma omp barrier
        }
    }

    gettimeofday(&tv1, &tz1);
    
    // assert(x==y);
    // assert(x==N*n_threads);
    printf("time: %ld microseconds\n",(tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    // tid = (pthread_t*)malloc(num_threads*sizeof(pthread_t));

    return 0;
}
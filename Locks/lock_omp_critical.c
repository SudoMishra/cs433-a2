#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>
#include <assert.h>

#define N 10000000

int x = 0, y = 0;
int n_threads;

void critical()
{
    int i;
    for (i=0; i<N; i++) {
        #pragma omp critical
        {
            assert (x == y);
            x = y + 1;
            y++;
        }
    }
}

int main(int argc, char *argv[])
{
    int i;
    struct timeval tv0,tv1;
    struct timezone tz0, tz1;

    if(argc!=2)
    {
        printf("Need no. of threads\n");
        exit(1);
    }

    n_threads = atoi(argv[1]);
    
    gettimeofday(&tv0, &tz0);

    #pragma omp parallel num_threads (n_threads) private(i)
        critical();
    
    gettimeofday(&tv1, &tz1);
    
    assert(x==y);
    assert(x==N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}
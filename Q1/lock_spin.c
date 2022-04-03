#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (10000000)
int num_threads;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int x = 0, y = 0;
int* choosing;
int* ticket;
int lock = 0;

unsigned char CompareAndSet(int oldVal, int newVal, int *ptr) 
{
    	int oldValOut;
    	unsigned char result;
    	asm("lock cmpxchgl %4, %1 \n setzb %0"
                :"=qm"(result),  "+m" (*ptr), "=a" (oldValOut)
                :"a" (oldVal),  "r" (newVal)
                : );
    	return result;
}

void acquire(int *lock)
{
    while(!CompareAndSet(0,1,lock));
}

void release(int *lock)
{
    asm("":::"memory");
    *lock = 0;
}

// void acquire(int id)
// {
//     int j = 0;
//     choosing[id] = 1;
//     ticket[id] = max(ticket,num_threads*16) + 1;
//     choosing[id] = 0;
//     for(j=0;j<16*num_threads;j+=16)
//     {
//         while(choosing[j]);
//         while(check_ticket(j,id));
//     }
// }

// void release(int id)
// {
//     ticket[id] = 0;
// }


void *critical(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        // Acquire (&lock);
        // pthread_mutex_lock(&mutex);
	    // acquire(id);
        acquire(&lock);
        // assert (x == y);
        x = y + 1;
        y++;
        // release(id);
        release(&lock);
        // pthread_mutex_unlock(&mutex);
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
    choosing = (int*)malloc(num_threads*16*sizeof(int));
    ticket = (int*)malloc(num_threads*16*sizeof(int));
    for(i=0;i<num_threads*16;i+=16)
    {
        choosing[i] = 0;
        ticket[i] = 0;
    }
    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<num_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &id[i]);
   	}
    for (i=0; i<num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    // assert (x == y);
    // assert (x == N*num_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    printf("\n%lu bytes\n",sizeof(int));

    return 0;
}
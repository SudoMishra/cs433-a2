#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (10000000)
int num_threads;
int x = 0, y = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int* choosing;
int* ticket;
int max(int* ticket, int n)
{
    int i;
    int ans = ticket[0];
    asm("mfence":::"memory");
    for(i = 0;i<n;i=i+16)
    {
        if(ticket[i]>ans)
        {
            // asm("mfence":::"memory");
            ans=ticket[i];
            asm("mfence":::"memory");
        }
             
    }
    return ans;
}

int check_ticket(int j, int i)
{
    if(ticket[j])
    {
        if(ticket[j]<ticket[i])
            return 1;
        else if(ticket[j]==ticket[i] && j < i)
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

void acquire(int *id)
{
    int j = 0;
    choosing[*id] = 1;
    asm("mfence":::"memory");
    ticket[*id] = max(ticket,num_threads*16) + 1;
    asm("mfence":::"memory");
    choosing[*id] = 0;
    asm("mfence":::"memory");
    // asm("mfence":::"memory");
    for(j=0;j<16*num_threads;j+=16)
    {
        while(choosing[j]);
        while(check_ticket(j,*id));
    }
}

void release(int *id)
{
    
    // asm("mfence":::"memory");
    ticket[*id] = 0;
    asm("mfence":::"memory");
}


void *critical(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        // Acquire (&lock);
        // pthread_mutex_lock(&mutex);
	    acquire(&id);
        // assert (x == y);
        x = y + 1;
        y++;
        release(&id);
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
        id[i] = i*16;
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
    // printf("\n%d bytes",sizeof(int));

    return 0;
}
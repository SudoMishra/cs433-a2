#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (100000)

int ticket=1,rem_count=1,x=0,y=0;
int num_threads;
int FetchAndInc(int oldVal, int newVal, int *ptr) 
{
    	int oldValOut;
    	unsigned char result;
    	asm("lock cmpxchgl %4, %1 \n setzb %0"
                :"=qm"(result),  "+m" (*ptr), "=a" (oldValOut)
                :"a" (oldVal),  "r" (newVal)
                : );
        if(!result)
            return result;
        else
            return oldVal;
}

void acquire(int *ticket)
{
    int curr;
     do
    {
        curr = FetchAndInc(*ticket,*(ticket)+1,ticket);
        asm("mfence":::"memory");
    } while (!curr);
    while(rem_count<curr);
    // printf("ID : %d curr : %d\n",curr);
}

void release()
{
    // asm("mfence":::"memory");
    rem_count = rem_count+1;
    // printf("rem : %d\n",*rem_count);
    asm("mfence":::"memory");
    
}

void *critical(void* param)
{
    int i;
    for(int i=0;i<N;i++)
    {
        acquire(&ticket);
        // assert(x==y);
        x = y+1;
        y++;
        release();
    }
}
int main(int argc, char* argv[])
{
    int i,*id;
    pthread_t *tid;
    pthread_attr_t attr;
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
    if(argc!=2)
    {
        printf("Need no. of threads");
        exit(1);
    }
    num_threads = atoi(argv[1]);
    id = (int*)malloc(num_threads*sizeof(int));
    tid = (pthread_t*)malloc(num_threads*sizeof(pthread_t));
    for (i=0; i<num_threads; i++) 
        id[i] = i;
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<num_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &id[i]);
   	}
    for (i=0; i<num_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    // assert(x==y);
    // assert(x==N*num_threads);
    // printf("x:%d y%d\n",x,y);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    return 0;
}
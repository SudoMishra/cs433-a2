#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (10000000)

int num_threads;
int idx=1,x=0,y=0;
unsigned char *lock;
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

int acquire(int *idx)
{
    int curr;
     do
    {
        // asm("mfence":::"memory");
        int old = *idx;
        curr = FetchAndInc(old,(old+64)%(64*num_threads),idx);
        // asm("mfence":::"memory");
    } while (!curr);
    while(lock[curr]);
    // printf("Acquire ID : %d\n",curr);
    return curr;
}

void release(int curr)
{
    // asm("mfence":::"memory");
    lock[curr] = 1;
    asm("mfence":::"memory");
    // printf("Release ID : %d\n",curr);
    lock[(curr+64)%(64*num_threads)] = 0;
    asm("mfence":::"memory"); 
    
    
}

void *critical(void* param)
{
    int i, id = *(int *)param;
    int curr;
    for(int i=0;i<N;i++)
    {
        curr = acquire(&idx);
        // assert(x==y);
        x = y+1;
        y++;
        release(curr);
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
    lock = (unsigned char*)malloc((num_threads+1)*64*sizeof(unsigned char));
    for (i=0; i<num_threads; i++) 
        id[i] = i;
    for(i=0;i<64*num_threads;i=i+16)
    {
        lock[i+1] = 1;
    }
    lock[idx] = 0;
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
    // printf(" %lu bytes", sizeof(unsigned char));
    return 0;
}
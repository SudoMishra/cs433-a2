#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (1000000)

int n_threads;
int MAX_THREADS;

struct barrier_node 
{ 
       pthread_mutex_t count_lock; 
       pthread_cond_t ok_to_proceed_up; 
       pthread_cond_t ok_to_proceed_down; 
       int count; 
};
typedef struct barrier_node barrier_node;
barrier_node *bar;

void init_barrier(barrier_node *b) { 
    int i; 
    for (i = 0; i < MAX_THREADS; i++) { 
        (*b).count = 0; 
        pthread_mutex_init(&((*b).count_lock), NULL); 
        pthread_cond_init(&((*b).ok_to_proceed_up), NULL); 
        pthread_cond_init(&((*b).ok_to_proceed_down), NULL); 
    } 
}

void BARRIER(int *id)
{
    int i, base, index;
    i=2, base = 0;

    do
    {
        index = base + (*id) / i;
        if((*id) % i == 0)
        {
            pthread_mutex_lock(&bar[index].count_lock);
            bar[index].count ++;
            while( bar[index].count < 2)
            {
                pthread_cond_wait(&(bar[index].ok_to_proceed_up), &bar[index].count_lock);
            }
            pthread_mutex_unlock(&(bar[index].count_lock));
        }
        else
        {
            pthread_mutex_lock(&(bar[index].count_lock));
            bar[index].count++;
            if(bar[index].count == 2)
                pthread_cond_signal(&(bar[index].ok_to_proceed_up));
            while(pthread_cond_wait(&(bar[index].ok_to_proceed_down),&(bar[index].count_lock))!=0);
            pthread_mutex_unlock(&(bar[index].count_lock));
            break;
        }
        base = base + n_threads/i;
        i=i*2;
    }
    while( i <= n_threads);
    i = i/2;
    for(;i>1;i=i/2)
    {
        base = base - n_threads/i;
        index = base + (*id)/i;
        pthread_mutex_lock(&(bar[index].count_lock));
        bar[index].count = 0;
        pthread_cond_signal(&(bar[index].ok_to_proceed_down));
        pthread_mutex_unlock(&(bar[index].count_lock));
    }

}

void *critical(void *param)
{
    int i, id = *(int *)param;
    for(i=0;i<N;i++)
    {
        if(n_threads>1)
            BARRIER(&id);
        else
        {
            pthread_mutex_lock(&(bar[id].count_lock));
            pthread_mutex_unlock(&(bar[id].count_lock));
        }
    }
}

int main(int argc, char* argv[])
{
    
    int i,j, *id;
    pthread_t *tid;
    pthread_attr_t attr;
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
    
    // printf("%lu bytes", sizeof(pthread_mutex_t));
    if(argc != 2)
    {
        printf("Need no. of threads\n");
        exit(1);
    }
    n_threads = atoi(argv[1]);
    MAX_THREADS = n_threads;

    id = (int*)malloc(n_threads*16*sizeof(int));
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
    bar = (barrier_node*)malloc(n_threads*sizeof(barrier_node));

    for (i=0; i<n_threads; i++) 
        id[i*16] = i;
    
    for (i=0;i<n_threads;i++)
        init_barrier(&(bar[i]));
    
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &id[i*16]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}

    gettimeofday(&tv1, &tz1);
    
    printf("time: %ld microseconds\n",(tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}

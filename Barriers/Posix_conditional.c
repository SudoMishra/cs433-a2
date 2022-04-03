#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (1000000)

int n_threads;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv;
__thread int local_sense = 0;

struct bar_type {
   int counter;
   pthread_mutex_t *lock;
   pthread_cond_t *cv;
//    int flag;
};
typedef struct bar_type barrier;

void bar_init(barrier *bar) 
{
   (*bar).lock = &mutex;
   (*bar).counter = 0;
   (*bar).cv = &cv;
   pthread_cond_init ((*bar).cv, NULL);
//    (*bar).flag = 0;
}

void BARRIER (barrier *bar) {
   pthread_mutex_lock((*bar).lock);
   (*bar).counter++;
   if ((*bar).counter == n_threads) {
      (*bar).counter = 0;
      pthread_cond_broadcast((*bar).cv);
   }
   else pthread_cond_wait((*bar).cv, (*bar).lock);
   pthread_mutex_unlock((*bar).lock);
}


// void BARRIER(barrier *bar) {
//     local_sense = !local_sense; /* this is private per processor */
//     pthread_mutex_lock((*bar).lock);
//     (*bar).counter++;
//     if ((*bar).counter == n_threads) 
//     {
//       pthread_mutex_unlock((*bar).lock);
//       (*bar).counter = 0;
//       (*bar).flag = local_sense;
//     }
//     else 
//     {
//        pthread_mutex_unlock((*bar).lock);
//        while ((*bar).flag != local_sense);
//     }
// }


void *critical(void *param)
{
    int i;
    barrier *bar = (barrier*)param;
    for (i=0; i<N; i++) 
    {
        BARRIER(bar);
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
    barrier bar;
    bar_init(&bar);
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &bar);
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
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#define N (1000000)

int n_threads;
int MAX;
unsigned char **flag;

void TreeBarrier (int *pid) {
   unsigned int i, mask;

   for (i = 0, mask = 1; (mask & *pid) != 0; ++i, mask <<= 1) {
      while (!flag[(*pid)][i*64]);
      flag[(*pid)][i*64] = 0;
   }
   if (*pid < (n_threads - 1)) {
      flag[(*pid) + mask][i*64] = 1; 
      while (!flag[(*pid)][(MAX- 1)*64]);
      flag[(*pid)][(MAX - 1)*64] = 0;
   }
   for (mask >>= 1; mask > 0; mask >>= 1) {
      flag[((*pid)- mask)][(n_threads-1)*64] = 1; 
   }
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
    int i, id = *(int *)param;
    // barrier *bar = (barrier*)param;
    for (i=0; i<N; i++) 
    {
        TreeBarrier(&id);
        // sBARRIER(bar);
    }

}



int main(int argc, char* argv[])
{
    
    int i,j, *id;
    pthread_t *tid;
    pthread_attr_t attr;
    struct timeval tv0, tv1;
    struct timezone tz0, tz1;
    // printf("%lu bytes", sizeof(pthread_cond_t));
    if(argc != 2)
    {
        printf("Need no. of threads\n");
        exit(1);
    }
    n_threads = atoi(argv[1]);
    MAX = n_threads;
    id = (int*)malloc(n_threads*16*sizeof(int));
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
    for (i=0; i<n_threads; i++) 
        id[i*16] = i;
    flag = (unsigned char **)malloc(n_threads*sizeof(unsigned char*));
    for(i=0;i<n_threads;i++)
    flag[i] = (unsigned char *)malloc(n_threads*64*sizeof(unsigned char));
    for(i=0;i<n_threads;i++)
    {
        for(j=0;j<n_threads;j++)
        {
            flag[i][j*64] = 0;
        }
    }
    // barrier bar;
    // bar_init(&bar);

    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical, &id[i*16]);
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




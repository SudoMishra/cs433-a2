#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <semaphore.h>
#include <omp.h>

#define N (10000000)
#define M (1000000)

// Array lock

int n_threads;
int idx=1,x=0,y=0;
unsigned char *lock;
int FetchAndInc_array(int oldVal, int newVal, int *ptr) 
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

int acquire_array(int *idx)
{
    int curr;
     do
    {
        // asm("mfence":::"memory");
        int old = *idx;
        curr = FetchAndInc_array(old,(old+64)%(64*n_threads),idx);
        // asm("mfence":::"memory");
    } while (!curr);
    while(lock[curr]);
    // printf("Acquire ID : %d\n",curr);
    return curr;
}

void release_array(int curr)
{
    // asm("mfence":::"memory");
    lock[curr] = 1;
    asm("mfence":::"memory");
    // printf("Release ID : %d\n",curr);
    lock[(curr+64)%(64*n_threads)] = 0;
    asm("mfence":::"memory"); 
    
    
}

void *critical_array(void* param)
{
    int i, id = *(int *)param;
    int curr;
    for(int i=0;i<N;i++)
    {
        curr = acquire_array(&idx);
        // assert(x==y);
        x = y+1;
        y++;
        release_array(curr);
    }
}
int main_array(int argc, char* argv[])
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
    n_threads = atoi(argv[1]);
    id = (int*)malloc(n_threads*sizeof(int));
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
    lock = (unsigned char*)malloc((n_threads+1)*64*sizeof(unsigned char));
    for (i=0; i<n_threads; i++) 
        id[i] = i;
    for(i=0;i<64*n_threads;i=i+16)
    {
        lock[i+1] = 1;
    }
    lock[idx] = 0;
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_array, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    // assert(x==y);
    // assert(x==N*n_threads);
    // printf("x:%d y%d\n",x,y);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    // printf(" %lu bytes", sizeof(unsigned char));
    return 0;
}


/*lamport   */


int* choosing;
int* ticket_lamport;
int max_lamport(int* ticket_lamport, int n)
{
    int i;
    int ans = ticket_lamport[0];
    asm("mfence":::"memory");
    for(i = 0;i<n;i=i+16)
    {
        if(ticket_lamport[i]>ans)
        {
            // asm("mfence":::"memory");
            ans=ticket_lamport[i];
            asm("mfence":::"memory");
        }
             
    }
    return ans;
}

int check_ticket_lamport(int j, int i)
{
    if(ticket_lamport[j])
    {
        if(ticket_lamport[j]<ticket_lamport[i])
            return 1;
        else if(ticket_lamport[j]==ticket_lamport[i] && j < i)
            return 1;
        else
            return 0;
    }
    else
        return 0;
}

void acquire_lamport(int *id)
{
    int j = 0;
    choosing[*id] = 1;
    asm("mfence":::"memory");
    ticket_lamport[*id] = max_lamport(ticket_lamport,n_threads*16) + 1;
    asm("mfence":::"memory");
    choosing[*id] = 0;
    asm("mfence":::"memory");
    // asm("mfence":::"memory");
    for(j=0;j<16*n_threads;j+=16)
    {
        while(choosing[j]);
        while(check_ticket_lamport(j,*id));
    }
}

void release_lamport(int *id)
{
    
    // asm("mfence":::"memory");
    ticket_lamport[*id] = 0;
    asm("mfence":::"memory");
}


void *critical_lamport(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
	    acquire_lamport(&id);
        // assert (x == y);
        x = y + 1;
        y++;
        release_lamport(&id);

    }
}

int main_lamport(int argc, char* argv[])
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
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
	// private_sum = (double*)malloc(n_threads*sizeof(double));
	id = (int*)malloc(n_threads*sizeof(int));
 	for (i=0; i<n_threads; i++) 
        id[i] = i*16;
    choosing = (int*)malloc(n_threads*16*sizeof(int));
    ticket_lamport = (int*)malloc(n_threads*16*sizeof(int));
    for(i=0;i<n_threads*16;i+=16)
    {
        choosing[i] = 0;
        ticket_lamport[i] = 0;
    }
    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_lamport, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    // assert (x == y);
    // assert (x == N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    // printf("\n%d bytes",sizeof(int));

    return 0;
}

// Ticket Lock

int ticket=1,rem_count=1;
// int n_threads;
int FetchAndInc_ticket(int oldVal, int newVal, int *ptr) 
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

void acquire_ticket(int *ticket)
{
    int curr;
     do
    {
        int old_val = *ticket;
        curr = FetchAndInc_ticket(old_val,old_val+1,ticket);
        asm("mfence":::"memory");
    } while (!curr);
    
    while(rem_count<curr);
    // printf("ID : %d curr : %d\n",rem_count,curr);
}

void release_ticket()
{
    // asm("mfence":::"memory");
    rem_count = rem_count+1;
    // printf("rem : %d\n",*rem_count);
    asm("mfence":::"memory");
    
}

void *critical_ticket(void* param)
{
    int i;
    for(i=0;i<N;i++)
    {
        acquire_ticket(&ticket);
        // assert(x==y);
        x = y+1;
        y++;
        asm("mfence":::"memory");
        release_ticket();
    }
}
int main_ticket(int argc, char* argv[])
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
    n_threads = atoi(argv[1]);
    id = (int*)malloc(n_threads*sizeof(int));
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
    for (i=0; i<n_threads; i++) 
        id[i] = i;
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_ticket, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    // assert(x==y);
    // assert(x==N*n_threads);
    // printf("x:%d y%d\n",x,y);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    return 0;
}


// Omp critical

void critical_omp()
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

int main_omp(int argc, char *argv[])
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
        critical_omp();
    
    gettimeofday(&tv1, &tz1);
    
    assert(x==y);
    assert(x==N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}

// Posix lock

// int n_threads;
pthread_mutex_t mutex_posix = PTHREAD_MUTEX_INITIALIZER;
// int x = 0, y = 0;

void *critical_posix(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        // Acquire (&lock);
        pthread_mutex_lock(&mutex_posix);
	    assert (x == y);
        x = y + 1;
        y++;
        pthread_mutex_unlock(&mutex_posix);
        // Release (&lock);
    }
}

int main_posix(int argc, char* argv[])
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
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
	// private_sum = (double*)malloc(n_threads*sizeof(double));
	id = (int*)malloc(n_threads*sizeof(int));
 	for (i=0; i<n_threads; i++) 
        id[i] = i;

    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_posix, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    assert (x == y);
    assert (x == N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}

// Semaphore lock

// int n_threads;
// pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// int x = 0, y = 0;
sem_t semaphore;

void *critical_sem(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        // Acquire (&lock);
        sem_wait(&semaphore);
	    {
            assert (x == y);
            x = y + 1;
            y++;
        }
        sem_post(&semaphore);
        // Release (&lock);
    }
}

int main_sem(int argc, char* argv[])
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
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
	// private_sum = (double*)malloc(n_threads*sizeof(double));
	id = (int*)malloc(n_threads*sizeof(int));
 	for (i=0; i<n_threads; i++) 
        id[i] = i;
    sem_init(&semaphore, 0, 1);

    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_sem, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    assert (x == y);
    assert (x == N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}

// Spin Lock

// int n_threads;
// // pthread_mutex_t mutex_spin = PTHREAD_MUTEX_INITIALIZER;
// int x = 0, y = 0;
// int* choosing;
// int* ticket;
int lock_spin = 0;

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

void acquire_spin(int *lock_spin)
{
    while(!CompareAndSet(0,1,lock_spin));
}

void release_spin(int *lock_spin)
{
    asm("":::"memory");
    *lock_spin = 0;
}

void *critical_spin(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        acquire_spin(&lock_spin);
        // assert (x == y);
        x = y + 1;
        y++;
        // release(id);
        release_spin(&lock_spin);
    }
}

int main_spin(int argc, char* argv[])
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
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
	// private_sum = (double*)malloc(n_threads*sizeof(double));
	id = (int*)malloc(n_threads*sizeof(int));
 	for (i=0; i<n_threads; i++) 
        id[i] = i;

    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_spin, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    // assert (x == y);
    // assert (x == N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
    printf("\n%lu bytes\n",sizeof(int));

    return 0;
}

// TTS

int lock_tts = 0;

unsigned char TestAndSet(int oldVal, int newVal, int *ptr)
{
    int oldValOut;
    unsigned char result;
    asm("lock cmpxchgl %4, %1 \n setzb %0"
                :"=qm"(result),  "+m" (*ptr), "=a" (oldValOut)
                :"a" (oldVal),  "r" (newVal)
                : );
    return result;
}

void acquire_tts(int *lock_tts)
{
    while(!TestAndSet(0,1,lock_tts))
    {
        // asm("":::"memory");
        while((*lock_tts));
    }
}
void release_tts(int *lock_tts)
{
    asm("":::"memory");
    *lock_tts=0;
}


void *critical_tts(void *param)
{
    int i, id = *(int*)(param);
    for (i=0; i<N; i++) {
        acquire_tts(&lock_tts);
        assert (x == y);
        x = y + 1;
        y++;
        release_tts(&lock_tts);
    }
}

int main_tts(int argc, char* argv[])
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
    tid = (pthread_t*)malloc(n_threads*sizeof(pthread_t));
	// private_sum = (double*)malloc(n_threads*sizeof(double));
	id = (int*)malloc(n_threads*sizeof(int));
 	for (i=0; i<n_threads; i++) 
        id[i] = i;
    gettimeofday(&tv0, &tz0);
    
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_tts, &id[i]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}
    gettimeofday(&tv1, &tz1);
    assert (x == y);
    assert (x == N*n_threads);
    printf("X: %d, Y: %d \ntime: %ld microseconds\n", x, y, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}

// Barrier Centralised

// int n_threads;

pthread_mutex_t mutex_cen_b = PTHREAD_MUTEX_INITIALIZER;
__thread int local_sense = 0;

struct bar_type {
   int counter;
   pthread_mutex_t *lock;
   int flag;
};
typedef struct bar_type barrier;

void bar_init_cen(barrier *bar) 
{
   (*bar).lock = &mutex_cen_b;
   (*bar).counter = 0;
   (*bar).flag = 0;
}

void BARRIER_cen(barrier *bar) {
    local_sense = !local_sense; /* this is private per processor */
    pthread_mutex_lock((*bar).lock);
    (*bar).counter++;
    if ((*bar).counter == n_threads) 
    {
      pthread_mutex_unlock((*bar).lock);
      (*bar).counter = 0;
      (*bar).flag = local_sense;
    }
    else 
    {
       pthread_mutex_unlock((*bar).lock);
       while ((*bar).flag != local_sense);
    }
}


void *critical_cen(void *param)
{
    int i;
    barrier *bar = (barrier*)param;
    for (i=0; i<M; i++) 
    {
        BARRIER_cen(bar);
    }

}



int main_cen(int argc, char* argv[])
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
    bar_init_cen(&bar);

    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_cen, &bar);
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

// Tree Barrier Busy

// int n_threads;
int MAX;
unsigned char **flag;

void TreeBarrier(int *pid) {
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

void *critical_tree_busy(void *param)
{
    int i, id = *(int *)param;
    // barrier *bar = (barrier*)param;
    for (i=0; i<M; i++) 
    {
        TreeBarrier(&id);
        // sBARRIER(bar);
    }

}



int main_tree_busy(int argc, char* argv[])
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
		pthread_create(&tid[i], &attr, critical_tree_busy, &id[i*16]);
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


// Posix Conditional

// int n_threads;

pthread_mutex_t mutex_pcond = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv;

struct bar_type_pcond {
   int counter;
   pthread_mutex_t *lock;
   pthread_cond_t *cv;
//    int flag;
};
typedef struct bar_type_pcond barrier_pcond;

void bar_init_pcond(barrier_pcond *bar) 
{
   (*bar).lock = &mutex_pcond;
   (*bar).counter = 0;
   (*bar).cv = &cv;
   pthread_cond_init ((*bar).cv, NULL);
//    (*bar).flag = 0;
}

void BARRIER_pcond(barrier_pcond *bar) {
   pthread_mutex_lock((*bar).lock);
   (*bar).counter++;
   if ((*bar).counter == n_threads) {
      (*bar).counter = 0;
      pthread_cond_broadcast((*bar).cv);
   }
   else pthread_cond_wait((*bar).cv, (*bar).lock);
   pthread_mutex_unlock((*bar).lock);
}


void *critical_pcond(void *param)
{
    int i;
    barrier_pcond *bar = (barrier_pcond*)param;
    for (i=0; i<M; i++) 
    {
        BARRIER_pcond(bar);
    }
}



int main_pcond(int argc, char* argv[])
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
    barrier_pcond bar;
    bar_init_pcond(&bar);
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_pcond, &bar);
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


// Tree Barrier Cond Vars

// int n_threads;
int MAX_THREADS;

struct barrier_node 
{ 
       pthread_mutex_t count_lock; 
       pthread_cond_t ok_to_proceed_up; 
       pthread_cond_t ok_to_proceed_down; 
       int count; 
};
typedef struct barrier_node barrier_node;
barrier_node *bar_cv;

void init_barrier(barrier_node *b) { 
    int i; 
    for (i = 0; i < MAX_THREADS; i++) { 
        (*b).count = 0; 
        pthread_mutex_init(&((*b).count_lock), NULL); 
        pthread_cond_init(&((*b).ok_to_proceed_up), NULL); 
        pthread_cond_init(&((*b).ok_to_proceed_down), NULL); 
    } 
}

void BARRIER_Tree_CV(int *id)
{
    int i, base, index;
    i=2, base = 0;

    do
    {
        index = base + (*id) / i;
        if((*id) % i == 0)
        {
            pthread_mutex_lock(&bar_cv[index].count_lock);
            bar_cv[index].count ++;
            while( bar_cv[index].count < 2)
            {
                pthread_cond_wait(&(bar_cv[index].ok_to_proceed_up), &bar_cv[index].count_lock);
            }
            pthread_mutex_unlock(&(bar_cv[index].count_lock));
        }
        else
        {
            pthread_mutex_lock(&(bar_cv[index].count_lock));
            bar_cv[index].count++;
            if(bar_cv[index].count == 2)
                pthread_cond_signal(&(bar_cv[index].ok_to_proceed_up));
            while(pthread_cond_wait(&(bar_cv[index].ok_to_proceed_down),&(bar_cv[index].count_lock))!=0);
            pthread_mutex_unlock(&(bar_cv[index].count_lock));
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
        pthread_mutex_lock(&(bar_cv[index].count_lock));
        bar_cv[index].count = 0;
        pthread_cond_signal(&(bar_cv[index].ok_to_proceed_down));
        pthread_mutex_unlock(&(bar_cv[index].count_lock));
    }

}

void *critical_Tree_CV(void *param)
{
    int i, id = *(int *)param;
    for(i=0;i<M;i++)
    {
        if(n_threads>1)
            BARRIER_Tree_CV(&id);
        else
        {
            pthread_mutex_lock(&(bar_cv[id].count_lock));
            pthread_mutex_unlock(&(bar_cv[id].count_lock));
        }
    }
}

int main_cv(int argc, char* argv[])
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
    bar_cv = (barrier_node*)malloc(n_threads*sizeof(barrier_node));

    for (i=0; i<n_threads; i++) 
        id[i*16] = i;
    
    for (i=0;i<n_threads;i++)
        init_barrier(&(bar_cv[i]));
    
    gettimeofday(&tv0, &tz0);
    pthread_attr_init(&attr);
    for (i=0; i<n_threads; i++) {
		pthread_create(&tid[i], &attr, critical_Tree_CV, &id[i*16]);
   	}
    for (i=0; i<n_threads; i++) {
		pthread_join(tid[i], NULL);
	}

    gettimeofday(&tv1, &tz1);
    
    printf("time: %ld microseconds\n",(tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));

    return 0;
}

// POSIX Barrier Interface

pthread_barrier_t pbar;

void *critical_pbar(void *param)
{
    int i;
    pthread_barrier_t *pbar = (pthread_barrier_t*)param;
    for (i=0; i<M; i++) 
    {
        pthread_barrier_wait(pbar);
    }
}



int main_pbar(int argc, char* argv[])
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
		pthread_create(&tid[i], &attr, critical_pbar, &pbar);
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

// OMP Critical

int main_omp_bar(int argc, char* argv[])
{
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
        for (i=0; i<M; i++) 
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

int main(int argc, char*argv[])
{
    // main_array(argc,argv);
    // main_cen(argc,argv);
    // main_cv(argc,argv);
    // main_lamport(argc,argv);
    // main_pbar(argc,argv);
    // main_posix(argc,argv);
    // main_spin(argc,argv);
    // main_ticket(argc,argv);
    // main_tree_busy(argc,argv);
    // main_tts(argc,argv);
    // main_omp(argc,argv);
    // main_omp_bar(argc,argv);
    // main_pcond(argc,argv);
    return 0;
}
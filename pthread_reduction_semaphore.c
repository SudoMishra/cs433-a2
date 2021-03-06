#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <semaphore.h>

#define SIZE (1<<30)

double sum = 0;
float *a;
int num_threads;
double *private_sum;
sem_t semaphore;

void *solver (void *param)
{
	int i, id = *(int*)(param);
	private_sum[id] = 0;
	
	for (i=(SIZE/num_threads)*id; i<(SIZE/num_threads)*(id+1); i++) {
		private_sum[id] += (a[i]*a[i]);
	}

	sem_wait(&semaphore);
	sum += private_sum[id];
	sem_post(&semaphore);
}	

int main (int argc, char *argv[])
{
	int i, *id;
	pthread_t *tid;
	pthread_attr_t attr;
	struct timeval tv0, tv1;
	struct timezone tz0, tz1;

	if (argc != 2) {
		printf ("Need number of threads.\n");
		exit(1);
	}
	num_threads = atoi(argv[1]);
	tid = (pthread_t*)malloc(num_threads*sizeof(pthread_t));
	private_sum = (double*)malloc(num_threads*sizeof(double));
	id = (int*)malloc(num_threads*sizeof(int));
 	for (i=0; i<num_threads; i++) id[i] = i;

	sem_init(&semaphore, 0, 1);

 	a = (float*)malloc(sizeof(float)*SIZE);
    	assert(a != NULL);
	for (i=0; i<SIZE; i++) a[i] = 1;

	pthread_attr_init(&attr);

	gettimeofday(&tv0, &tz0);
	for (i=1; i<num_threads; i++) {
		pthread_create(&tid[i], &attr, solver, &id[i]);
   	}
	for (i=0; i<SIZE/num_threads; i++) {
		private_sum[0] += a[i];
	}
	
	sem_wait(&semaphore);
	sum += private_sum[0];
	sem_post(&semaphore);

	for (i=1; i<num_threads; i++) {
		pthread_join(tid[i], NULL);
	}

	gettimeofday(&tv1, &tz1);
	sem_destroy(&semaphore);
	printf("SUM: %lf, time: %ld microseconds\n", sum, (tv1.tv_sec-tv0.tv_sec)*1000000+(tv1.tv_usec-tv0.tv_usec));
	return 0;
}
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>


#define NUMBER_OF_THREADS 10

sem_t semaphore;
pthread_barrier_t thread_barrier;
int * counter;

#define CHECK_ERR(a,msg) {if ((a) == -1) { perror((msg)); exit(EXIT_FAILURE); } }
#define CHECK_ERR_MMAP(a,msg) {if ((a) == MAP_FAILED) { perror((msg)); exit(EXIT_FAILURE); } }


void * thread_function(void * arg) {
	if (sem_wait(&semaphore) == -1) {
		perror("sem_wait");
		exit(EXIT_FAILURE);
	}

	// Mutex
  (*counter)++;
  printf("Incrementing... Counter: %d\n", *counter);

	if (sem_post(&semaphore) == -1) {
		perror("sem_post");
		exit(EXIT_FAILURE);
	}

	int s = pthread_barrier_wait(&thread_barrier);
	if (!(s == PTHREAD_BARRIER_SERIAL_THREAD || s == 0)){
		perror("pthread barrier error");
		exit(EXIT_FAILURE);
	}

  printf("Value of counter: %d\n", *counter);

	return NULL;
}

int main(int argc, char *argv[]) {
	pthread_t *t;
	t = malloc(sizeof(pthread_t));
	int res = 0;

  counter = mmap(NULL, // NULL: è il kernel a scegliere l'indirizzo
			sizeof(int), // dimensione della memory map
			PROT_READ | PROT_WRITE, // memory map leggibile e scrivibile
			MAP_SHARED | MAP_ANONYMOUS, // memory map condivisibile con altri processi e senza file di appoggio
			-1,
			0); // offset nel file
	CHECK_ERR_MMAP(counter,"mmap")

	res = sem_init(&semaphore, 0, 1);
	CHECK_ERR(res,"sem_init")

	res = pthread_barrier_init(&thread_barrier, NULL, NUMBER_OF_THREADS);
	CHECK_ERR(res,"pthread_create()")

	for(int i=0; i<NUMBER_OF_THREADS; i++){
		t = realloc(t, (i+1)*sizeof(pthread_t));
		res = pthread_create(&t[i], NULL, thread_function, NULL);
		CHECK_ERR(res,"pthread_create()")
	}

	for(int i=0; i<NUMBER_OF_THREADS; i++){
		res = pthread_join(t[i], NULL);
		CHECK_ERR(res,"pthread_join()")
	}

	res = sem_destroy(&semaphore);
	CHECK_ERR(res,"sem_destroy")

	res = pthread_barrier_destroy(&thread_barrier);
	CHECK_ERR(res,"pthread_barrier_destroy() error")

	return EXIT_SUCCESS;
}
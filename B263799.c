// PPLS Exercise 1 Starter File
//
// See the exercise sheet for details
//
// Note that NITEMS, NTHREADS and SHOWDATA should
// be defined at compile time with -D options to gcc.
// They are the array length to use, number of threads to use
// and whether or not to printout array contents (which is
// useful for debugging, but not a good idea for large arrays).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> // in case you want semaphores, but you don't really
                       // need them for this exercise


              
                       
// Structure for housing the thread characteristics
typedef struct {
  int ID;
  int *data;
  int start;
  int end;
  int base_size;
  int remainder;
  pthread_barrier_t *barrier1;
  pthread_barrier_t *barrier2;
} thread_args;

// Print a helpful message followed by the contents of an array
// Controlled by the value of SHOWDATA, which should be defined
// at compile time. Useful for debugging.
void showdata (char *message,  int *data,  int n) {
  int i; 

  if (SHOWDATA) {
      printf ("%s", message);
      for (i=0; i<n; i++ ){
      printf (" %d", data[i]);
      }
      printf("\n");
    }
}

// Check that the contents of two integer arrays of the same length are equal
// and return a C-style boolean
int checkresult (int* correctresult,  int *data,  int n) {
  int i; 

  for (i=0; i<n; i++ ){
    if (data[i] != correctresult[i]) return 0;
  }
  return 1;
}

// Compute the prefix sum of an array **in place** sequentially
void sequentialprefixsum (int *data, int n) {
  int i;

  for (i=1; i<n; i++ ) {
    data[i] = data[i] + data[i-1];
  }
}


static void *worker(void *arg) {
  thread_args *args = (thread_args *) arg;
  int *data = args->data;
  int start = args->start;
  int end = args->end;
  int ID = args->ID;
  int base_size = args->base_size;
  int remainder = args->remainder;

  // Phase 1: Local prefix sum
  for (int i = start + 1; i < end; i++) {
      data[i] += data[i - 1];
  }

  // Synchronize all threads after phase 1
  pthread_barrier_wait(args->barrier1);

  // Phase 2: Only thread 0 processes
  if (ID == 0) {
      int total_chunks = NTHREADS;
      int *last_elements = malloc(total_chunks * sizeof(int));
      if (!last_elements) {
          perror("malloc failed");
          exit(EXIT_FAILURE);
      }

      // Last element manipulation and summations
      for (int j = 0; j < total_chunks; j++) {
          int chunk_start = j * base_size;
          int chunk_end = (j == total_chunks - 1) ? chunk_start + base_size + remainder : chunk_start + base_size;
          last_elements[j] = data[chunk_end - 1];
          last_elements[j] += last_elements[j - 1]; // Compute prefix sum on the collected last elements
          data[chunk_end - 1] = last_elements[j]; // Update each chunk's last element   
      }
      free(last_elements);
  }

  // Synchronize all threads after phase 2
  pthread_barrier_wait(args->barrier2);

  // Phase 3: Add previous chunk's last element to current chunk (except thread 0)
  if (ID != 0) {
      int prev_value = data[ID * base_size - 1];
      for (int i = start; i < end - 1; i++) {
          data[i] += prev_value;
      }
  }

  free(arg);
  return NULL;
}

// YOU MUST WRITE THIS FUNCTION AND ANY ADDITIONAL FUNCTIONS YOU NEED
void parallelprefixsum(int *data, int n) {
  if (n < NTHREADS) {
      fprintf(stderr, "NITEMS must be >= NTHREADS\n");
      exit(EXIT_FAILURE);
  }

  int base_size = n / NTHREADS;
  int remainder = n % NTHREADS;

  pthread_t threads[NTHREADS]; //thread handles to init the array of threads of size NTHREADS
  pthread_barrier_t barrier1, barrier2; //barriers for capturing threads during phase 1 and phase 2

  if (pthread_barrier_init(&barrier1, NULL, NTHREADS) != 0 || pthread_barrier_init(&barrier2, NULL, NTHREADS) != 0) {
      perror("pthread_barrier_init failed");
      exit(EXIT_FAILURE);
  }
  int i;
  for (i = 0; i < NTHREADS; i++) {
      thread_args *arg = malloc(sizeof(thread_args));
      if (!arg) {
          perror("malloc failed");
          exit(EXIT_FAILURE);
      }
      arg->ID = i;
      arg->data = data;
      arg->base_size = base_size;
      arg->remainder = remainder;
      arg->start = i * base_size;
      arg->end = (i == NTHREADS - 1) ? arg->start + base_size + remainder : arg->start + base_size;
      arg->barrier1 = &barrier1;
      arg->barrier2 = &barrier2;

      if (pthread_create(&threads[i], NULL, worker, arg) != 0) {
          perror("pthread_create failed");
          exit(EXIT_FAILURE);
      }
  }

  for (int i = 0; i < NTHREADS; i++) {
      if (pthread_join(threads[i], NULL) != 0) {
          perror("pthread_join failed");
          exit(EXIT_FAILURE);
      }
  }

  pthread_barrier_destroy(&barrier1);
  pthread_barrier_destroy(&barrier2);
}


int main (int argc, char* argv[]) {

  int *arr1, *arr2, i;

  // Check that the compile time constants are sensible
  if ((NITEMS>10000000) || (NTHREADS>32)) {
    printf ("So much data or so many threads may not be a good idea! .... exiting\n");
    exit(EXIT_FAILURE);
  }

  // Create two copies of some random data
  arr1 = (int *) malloc(NITEMS*sizeof(int));
  arr2 = (int *) malloc(NITEMS*sizeof(int));
  srand((int)time(NULL));
  for (i=0; i<NITEMS; i++) {
     arr1[i] = arr2[i] = rand()%5;
  }
  showdata ("initial data          : ", arr1, NITEMS);

  // Calculate prefix sum sequentially, to check against later on
  sequentialprefixsum (arr1, NITEMS);
  showdata ("sequential prefix sum : ", arr1, NITEMS);

  // Calculate prefix sum in parallel on the other copy of the original data
  parallelprefixsum (arr2, NITEMS);
  showdata ("parallel prefix sum   : ", arr2, NITEMS);

  // Check that the sequential and parallel results match
  if (checkresult(arr1, arr2, NITEMS))  {
    printf("Well done, the sequential and parallel prefix sum arrays match.\n");
  } else {
    printf("Error: The sequential and parallel prefix sum arrays don't match.\n");
  }

  free(arr1); free(arr2);
  return 0;
}

/*
Discussion on implementation of threads using pthread for the algorithm:


The logic that I have applied here is pretty straight forward.
The parallelprefixsum() function called inside the main function 
houses the pthread methods and a custom function - worker().
Initially a thread handler array "threads" is created, of size 
NTHREADS. I also made use of the pthread_barrier_t data type to 
create barriers, barrier1 and barrier2, to synchronize the threads
between the phases of the algorithm.

Threads are created using the pthread_create method. The function worker() is 
passed in it as the start for the threads to work on.
A struct thread_args, is used to hold the thread arguments which include 
the thread ID, thread data, start and end indices for each block that 
the thread caters to, the base_size which is every block's size 
in the problem, remainder being the number of elements that 
should be accounted in the last thread. Inside the worker function this 
struct is used for every individual thread. 

The phase 1 is a local prefix sum and is pretty straightforward.
pthread_barrier_wait method is used to sync up the threads via the barrier1.
This waits for all threads and goes to the phase 2 where only thread 0 is
allowed to do the summation operation for the last element in each block, 
excluding the first block.

Once that is done, sync the threads this time using barrier2
and send it to phase 3. Here the final element from the previous block
is added to each element in the current block of the data array.
Once this is done, NULL is returned back to parallelprefixsum(). 
The threads are joined with the main thread using pthread_join method and 
the barriers are destroyed using pthread_barrier_destroy method. 

Resources are released whenever and wherever necessary using the free() method.
*/
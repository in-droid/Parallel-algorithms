#include <pthread.h>
#include<stdio.h>
#include <omp.h>
#include <time.h>
#include <stdlib.h>


#define T 16
#define TABLE_SIZE 1000000

void *thread_function(void* );
void create_threads();
void swap(int i, int j, int* A);
void print_table(int *table);
void fill_table();
int test_sort(int * array);
int isSorted();

pthread_t nit[T];
pthread_barrier_t barrier;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct params
{
    int id;
    int comparisonsEven;
    int comparisonsOdd;
    int arraySize;
    int startEven;
    int startOdd;
    int blockSorted;
    int *array;
};

struct params argumenti[T];
static int allSwaps;
int table[TABLE_SIZE];



int main () {
    allSwaps = 0;
    fill_table();

    double startTime = omp_get_wtime();
    pthread_barrier_init(&barrier, NULL, T);

    create_threads(table, TABLE_SIZE);

    for(int j = 0; j < T; j++) {
        pthread_join(nit[j], NULL);
    }
    double endTime = omp_get_wtime();
    printf("------------------\n");
    if (test_sort(table)) {
        printf("SUCCESS\n");
        printf("It took %f seconds to sort the numbers\n", endTime - startTime);
    }
    else {
        printf("TEST FAILED\n");
    }
     
    pthread_mutex_destroy(&mutex);
    pthread_barrier_destroy(&barrier);
    
    return 0;
}


void* thread_function(void *args) {
    struct params *argumenti = (struct params *) args;
    
    int id = argumenti->id;
    int swapped = 0;
    int swappedPrev = 1;
    int startEven = argumenti->startEven;
    int startOdd = argumenti->startOdd;
    int* array = argumenti->array;
    int arraySize = argumenti->arraySize;
    int comparisonsEven = argumenti->comparisonsEven;
    int comparisonsOdd = argumenti->comparisonsOdd;
   
    for(int j = 0; j < arraySize; j++) {
        if (j > 0) {
            swappedPrev = swapped;
        } 
        swapped = 0;
        if (j % 2 == 0) {
            int tmpInd = startEven;
            for(int i = 0; i < comparisonsEven; i++) {
                if(array[tmpInd] > array[tmpInd + 1]) {
                    swap(tmpInd, tmpInd + 1, array);
                    swapped = 1;
                }
                tmpInd += 2;
            }
        }
        else {
            int tmpInd = startOdd;
            for(int i = 0; i < comparisonsOdd; i++) {
                if(array[tmpInd] > array[tmpInd + 1]) {
                    swap(tmpInd, tmpInd + 1, array);
                    swapped = 1;
                }
                tmpInd += 2;
            }
        }
        
        if (j > 0) {
            int temp = swappedPrev | swapped;
            pthread_mutex_lock(&mutex);
            allSwaps |= temp;
            pthread_mutex_unlock(&mutex);
        }
           
        pthread_barrier_wait(&barrier);
        if (!allSwaps && j > 0) {
            printf("ITERATIONS: %d\n", j);
            break;
        }
        
    }
}

void create_threads(int *array, int size) {
    int start = 0;
    for(int i = 0; i < T; i++) {
        int id = i + 1;
        int totalOdd;
        argumenti[i].id = id;
        int comparisonsEven = (id + 1) * size / (2 * T) - id * size / (2 * T);
        if (size % 2 == 0) {
            totalOdd = (size / 2) - 1;
        }
        else {
            totalOdd = size / 2;
        }
        
        int comparisonsOdd = (id + 1) * (totalOdd) / T - id * (totalOdd) / T;
        argumenti[i].arraySize = size;
        if (i == 0) {
            argumenti[i].startEven = 0;
            argumenti[i].startOdd = 1;    
        }
        else {
            argumenti[i].startEven = argumenti[i-1].startEven + 2 * argumenti[i-1].comparisonsEven;
            argumenti[i].startOdd = argumenti[i-1].startOdd + 2 * argumenti[i-1].comparisonsOdd;
        }
        argumenti[i].comparisonsEven = comparisonsEven;
        argumenti[i].comparisonsOdd = comparisonsOdd;
        argumenti[i].array = array;
        argumenti[i].blockSorted = 0;

        pthread_create(&nit[i], NULL, thread_function, (void *) &argumenti[i]);
    } 
}

void swap(int i, int j, int* A) {
    int temp = A[i];
    A[i] = A[j];
    A[j] = temp;
}


void print_table(int *array) {
    for(int i = 0; i < TABLE_SIZE; i++) {
        printf("%d\t", array[i]);
    }
    printf("\n");
}


void fill_table() {
    srand(time(NULL));
    for(int i=0; i < TABLE_SIZE; i++) {
       table[i] = rand();
       //table[i] = i;
    }
}

int test_sort(int *array) {
    for(int i=0; i < TABLE_SIZE - 1; i++) {
        if (array[i] > array[i + 1]) {
            return 0;
        }
    }
    return 1;
}


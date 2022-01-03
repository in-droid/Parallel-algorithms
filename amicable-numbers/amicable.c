#include<stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <math.h>

#define T 16

int sum_devisors(int n);
int amicable_sum_parallel(int N);
int amicable_sum_serial(int N);

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Invalid number of arguments.\n");
        return 0;
    }
    int N = atoi(argv[1]);
    double startTime = omp_get_wtime();
    printf("\nPARALLEL ALGORITHM\n");
    printf("The sum of the americable numbers is: %d\n", amicable_sum_parallel(N));
    double endTime = omp_get_wtime();

    double parallelTime = endTime - startTime;
    printf("The parallel algorithm took %f seconds\n", parallelTime);
    printf("------------------------------------------------------\n");

    printf("SERIAL ALGORITHM\n");
    startTime = omp_get_wtime();
    printf("The sum of the americable numbers is: %d\n", amicable_sum_serial(N));
    endTime = omp_get_wtime();

    double serialTime = endTime - startTime;
    printf("The serial algorithm took %f seconds\n", serialTime);
    printf("------------------------------------------------------\n");
    printf("SPEEDUP FACTOR: %f\n", serialTime / parallelTime);

    return 0;
}

int amicable_sum_parallel(int N) {
    int *array = (int*) malloc((N + 2 ) * sizeof(int));
    int americableSum = 0;
    array[0] = 0;
    array[1] = 1;
    omp_set_num_threads(T);

    #pragma omp parallel for schedule(guided, 1)
    for(int i = 2; i < N; i++) {
        array[i] = sum_devisors(i);
    }

    #pragma omp parrallel shared(americableSum, array) for
    for(int i = 2; i < N; i++) {
        if (array[i] < N && array[i] != -1 
            && array[array[i]] == i && i != array[i]) {

            #pragma omp atomic update
            americableSum += array[i] + i;
            #pragma omp atomic write
            array[i] = -1;
        }
    }
    
    return americableSum;
}

int amicable_sum_serial(int N) {
    int *array = (int*) malloc((N + 2 ) * sizeof(int));
    int americableSum = 0;
    array[0] = 0;
    array[1] = 1;

    for(int i = 2; i < N; i++) {
        array[i] = sum_devisors(i);
    }

    for(int i = 2; i < N; i++) {
        if (array[i] < N && array[i] != -1 
            && array[array[i]] == i && i != array[i]) {
            //printf("ARRAY[i] = %d, i = %d\n", array[i], i);
            americableSum += array[i] + i;
            array[i] = -1;
        }
    }
    
    return americableSum;
}

int  sum_devisors(int n) {
    int sum = 0;
    for (int i = 2; i < sqrt(n); i++) {
            if (n % i == 0) {
                sum += i + (n / i);
            }
    }
    sum++;
    return sum;
}

/* THREADS = 4  N = 1M              | THREADS = 1           |  THREADS = 2                      |   THREADS = 8             |  THREADS = 16
    STATIC: SPEEDUP = 2.69          | SPEEDUP = 1.0003      |   STATIC: SPEEDUP = 1.53          |   STATIC: 5.71            |   STATIC 11.14
    DYNAMIC(100) SPEEDUP = 4.18     | .....                 |   DYNAMIC(100): SPEEDUP = 1.99    |   DYNAMIC(100): 8.53      |   DYNAMIC(100): 16.40
    DYNAMIC(1)   SPEEDUP = 4.19     |                       |   DYNAMIC(1):   SPEEDUP = 1.96    |   DYNAMIC(1):   8.26      |   DYNAMIC(1):   15.96
    DYNAMIC(1000) SPEEDUP = 4.26    |                       |   DYNAMIC(1000): SPEEDUP =  1.99  |   DYNAMIC(1000): 8.48     |   DYNAMIC(1000): 15.73
    GUIDED(1)     SPEEDUP = 4.18    |                       |   GUIDED(1):     SPEEDUP =  1.99  |   GUIDED(1):     8.52     |   GUIDED(1):     16.36
    GUIDED(100)   SPEEDUP = 4.27    |                       |   GUIDED(100):   SPEEDUP =        |                           |
    GUIDED(1000)  SPEEDUP = 4.27    |                       |   GUIDED(1000):  SPEEDUP =        |                           |
*/
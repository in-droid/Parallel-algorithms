#include <stdio.h>
#include <time.h>
#include <stdlib.h>


double * Random(int n) {
    srand(time(NULL));
    double * rez;
    rez = (double *)malloc(n * sizeof(double));
    
    for (int i = 0; i<n; i++) {
        int r = rand();
        rez[i] = (double) (r) / (double) (RAND_MAX);
    }

    return rez;
}

double ** Matrix(double *A, int n, int r, int c) {
    double ** result = (double **)malloc(r * sizeof(double *));
    /*
    c = n / r;
    if (n % r != 0) {
        c++;
    }
    */

    for (int i = 0; i < r; i++) {
        result[i] = (double *)calloc(c ,sizeof(double));
    }
    int size = sizeof(result[0]) / sizeof(double);

    for(int i = 0; i < n; i++) {
        int row = i / c;
        int column = i % c;
        result[row][column] = A[i];
    }

    return result;
}


double* Max(double* A, int n) {
    double* max = A;

    for(int i = 0; i < n; i++) {
        if (A[i] > *max) {
            max = &A[i];
        }
    }
    return max;
}

int main() {
    int n = 0;
    int r = 1;
    printf("Vnesi n: ");
    scanf("%d", &n);
    printf("Vnesi r: ");
    scanf("%d", &r);

    printf("1D\n");
    double * array = Random(n);
    for (int i = 0; i < n; i++)
    {
        printf("%.3f\t", array[i]);
    }
    printf("\n2D:\n");

    int c = n / r;
    if (n % r != 0) {
        c++;
    }

    double ** matrix = Matrix(array, n, r, c);
    for(int i = 0; i < r; i++) {
        for(int j = 0; j < c; j++) {
            printf("%.3f\t", matrix[i][j]);

        }
        printf("\n");
    }

    double *max = Max(array, n);
    printf("\n Najvecja vrednost: %.3f na naslovu: %p\n", *max, max);

    return 0;
}
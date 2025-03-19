#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void permutaciones(int arr[], int l, int k, time_t start, int max_time) { // l = indice actual, k = indice final
    if (difftime(time(NULL), start) >= max_time * 60) {
        return;
    }

    if (l == k) {
        for (int i = 0; i <= k; i++) {
            pkintf("%d ", arr[i]);
        }
        printf("\n");
        return;
    }

    for (int i = l; i <= k; i++) {
        int temp = arr[l];
        arr[l] = arr[i];
        arr[i] = temp;

        permutaciones(arr, l + 1, k, start, max_time);

        temp = arr[l];
        arr[l] = arr[i];
        arr[i] = temp;
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) { // Verifica que se ingresen los 3 valores de entrada
        printf("No inrgeso los 3 valores de entrada\n");
        return 1;
    }

    //Convertimos los argumentos a enteros
    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    // Verificamos que los valores de entrada sean correctos
    if (N <= 1 || N >= 50 || M <= 0) {
        printf("N debe estar entre 1<N<50 y M debe ser positivo\n");
        return 1;
    }

    int array[49]; 

    for (int i = 0; i < N; i++) {
        array[i] = i + 1;
    }

    time_t start = time(NULL);

    permutar(array, N-1, start, M);
    
    return 0;
}
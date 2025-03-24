#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void permutar(int arr[], int l, int k, time_t start, int max_time, long *Gracil) { // l = indice actual, k = indice final
    if (difftime(clock(), start)/CLOCKS_PER_SEC >= max_time*60) {
        printf("Se ha superado el tiempo maximo de ejecucion\n");
        return;
    }

    if (l == k) {
        int gracil = 1;
        for(int i = 0; i<k; i++){
            for(int j = i+1; j<k; j++){
                if (abs(arr[i] - arr[i + 1]) == abs(arr[j] - arr[j + 1])) {
                    gracil = 0;
                    break;
                }
            }
            if(!gracil) break;
        }
        if(gracil) (*Gracil)++;
        return;
    }

    for (int i = l; i <= k; i++) {
        int temp = arr[l];
        arr[l] = arr[i];
        arr[i] = temp;

        permutar(arr, l + 1, k, start, max_time, Gracil);

        temp = arr[l];
        arr[l] = arr[i];
        arr[i] = temp;
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) { // Verifica que se ingresen los 3 valores de entrada
        printf("No ingreso los 2 valores de entrada\n");
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

    int array[N];
    long graciles = 0;

    for (int i = 0; i < N; i++) {
        array[i] = i + 1;
    }

    clock_t start = clock();

    permutar(array, 0, N-1, start, M, &graciles);

    clock_t end = clock(); // Tiempo al finalizar

    // Calcula el tiempo transcurrido en minutos
    double Tiempo_seg = ((double)(end - start) / CLOCKS_PER_SEC);

    // Imprime los resultados
    printf("Numero ingresado por el usuario: %d\n", N);
    printf("Numero total de permutaciones graciles: %ld\n", graciles);
    printf("Tiempo total de ejecucion: %.6f segundos\n", Tiempo_seg);
    
    return 0;
}

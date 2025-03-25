#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int ver_gracil(int arr[], int n) {
    int diffs[n]; // Arreglo para marcar diferencias encontradas
    for (int i = 0; i < n; i++) diffs[i] = 0; // Inicializamos en 0

    for (int i = 0; i < n - 1; i++) {
        int diff = abs(arr[i + 1] - arr[i]); // Calculamos la diferencia absoluta
        if (diffs[diff]) { 
            return 0; // Si la diferencia está fuera de rango o se repite, no es grácil
        }
        diffs[diff] = 1; // Marcamos la diferencia como encontrada
    }
    return 1; // Si todas las diferencias son únicas, es grácil
}

void permutar(int arr[], int l, int k, time_t start, int max_time, long *gracil) { // l = indice actual, k = indice final
    if (difftime(clock(), start)/CLOCKS_PER_SEC >= max_time*60) {
        printf("Se ha superado el tiempo maximo de ejecucion\n");
        return;
    }

    if (l == k) { // Caso base: se generó una permutación completa
        if (ver_gracil(arr, k + 1)) { // Verificar si la permutación es grácil
            (*gracil)++;
        }
        return;
    }

    for (int i = l; i <= k; i++) {
        int temp = arr[l];
        arr[l] = arr[i];
        arr[i] = temp;

        permutar(arr, l + 1, k, start, max_time, gracil);

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

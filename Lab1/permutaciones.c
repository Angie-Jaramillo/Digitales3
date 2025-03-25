#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int ver_gracil(int arr[], int n) {
    char *diffs = (char *)calloc(n, sizeof(char)); // Memoria inicializada en 0
    if (!diffs) return 0; // En caso de fallo de memoria, asumimos no grácil

    for (int i = 0; i < n - 1; i++) {
        int diff = abs(arr[i + 1] - arr[i]);
        if (diffs[diff]) { 
            free(diffs);
            return 0; 
        }
        diffs[diff] = 1;
    }
    
    free(diffs);
    return 1;
}

void permutar(int arr[], int l, int k, time_t start, int max_time, long *gracil) {
    if (difftime(time(NULL), start) >= max_time * 60) {
        printf("Tiempo máximo alcanzado.\n");
        return;
    }

    if (l == k) { 
        *gracil += ver_gracil(arr, k + 1);
        return;
    }

    for (int i = l; i <= k; i++) {
        // Intercambio en línea sin variable temporal
        arr[l] ^= arr[i];
        arr[i] ^= arr[l];
        arr[l] ^= arr[i];

        permutar(arr, l + 1, k, start, max_time, gracil);

        // Deshacer el intercambio
        arr[l] ^= arr[i];
        arr[i] ^= arr[l];
        arr[l] ^= arr[i];
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) { 
        printf("Uso: %s <N> <M>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    if (N <= 1 || N >= 50 || M <= 0) {
        printf("N debe estar en 1 < N < 50 y M debe ser positivo.\n");
        return 1;
    }

    int array[N];
    long graciles = 0;
    
    for (int i = 0; i < N; i++) array[i] = i + 1;

    time_t start = time(NULL);

    permutar(array, 0, N - 1, start, M, &graciles);

    double tiempo_seg = difftime(time(NULL), start);

    printf("Número ingresado: %d\n", N);
    printf("Permutaciones gráciles: %ld\n", graciles);
    printf("Tiempo total: %.6f segundos\n", tiempo_seg);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void permutar(int array[], int N, int *contador, time_t start, int max_time){
    if (time(NULL) - start >= max_time) {
        printf("Se acabó el tiempo\n");
        return;
    }

    if (N == 0) {
        for (int i = 0; i < N; i++) {
            printf("%d ", array[i]);
        }
        printf("\n");
        *contador += 1;
        return;
    }

    for (int i = 0; i <= N; i++) {
        permutar(array, N - 1, contador, start, max_time);
        if (N % 2 == 0) {
            int temp = array[0];
            array[0] = array[N];
            array[N] = temp;
        } else {
            int temp = array[i];
            array[i] = array[N];
            array[N] = temp;
        }
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

    int contador = 0;
    time_t start = time(NULL);

    permutar(array, N - 1, &contador, start, M);
    
    return 0;
}
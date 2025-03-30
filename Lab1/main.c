/**
 * @file main.c
 * @brief Programa para encontrar permutaciones gráciles de un conjunto de números.
 * 
 * Este programa genera permutaciones gráciles de un conjunto de números,
 * asegurando que las diferencias entre elementos consecutivos sean únicas.
 * Además, el programa impone un límite de tiempo para la ejecución.
 * 
 * @authors Angie Jaramillo, Juan Manuel Rivera
 * @date 2025-03-29
 * @version 1.0
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>
 
 /**
  * @brief Encuentra permutaciones gráciles de forma recursiva.
  *
  * @param posicionActual Posición actual en el arreglo de permutaciones.
  * @param N Tamaño del conjunto de números.
  * @param arregloNumeros Arreglo que almacena la permutación actual.
  * @param numerosUsados Arreglo de seguimiento de números ya utilizados.
  * @param diferenciasUsadas Arreglo de seguimiento de diferencias utilizadas.
  * @param totalPermutaciones Contador de permutaciones válidas encontradas.
  * @param tiempoInicio Tiempo de inicio de la ejecución.
  * @param M Tiempo máximo de ejecución en minutos.
  */
 // Funcion recursiva para encontrar permutaciones graciles
 void encontrarPermutaciones(int posicionActual, int N, int arregloNumeros[], int numerosUsados[], int diferenciasUsadas[], long *totalPermutaciones, clock_t tiempoInicio, int M) {
     // Verificar si se ha superado el tiempo limite
     double tiempoTranscurrido = (double)(clock() - tiempoInicio) / CLOCKS_PER_SEC;
     if (tiempoTranscurrido >= M * 60) {
         return;
     }
     
     // Si se llenaron todas las posiciones, se encontro una permutacion valida
     if (posicionActual == N) {
         (*totalPermutaciones)++;
         return;
     }
     
     // Intentar colocar un numero en la posicion actual
     for (int numeroIntento = 1; numeroIntento <= N; numeroIntento++) {
         if (!numerosUsados[numeroIntento]) {  // Si el numero no ha sido usado
             // Si es la primera posicion o la diferencia es unica
             if (posicionActual == 0 || !diferenciasUsadas[abs(numeroIntento - arregloNumeros[posicionActual - 1])]) {
                 arregloNumeros[posicionActual] = numeroIntento;
                 numerosUsados[numeroIntento] = 1;
                 if (posicionActual > 0) {
                     int diferencia = abs(numeroIntento - arregloNumeros[posicionActual - 1]);
                     diferenciasUsadas[diferencia] = 1;
                 }
                 // Llamada recursiva para la siguiente posicion
                 encontrarPermutaciones(posicionActual + 1, N, arregloNumeros, numerosUsados, diferenciasUsadas, totalPermutaciones, tiempoInicio, M);
                 // Deshacer cambios para probar otra opcion
                 if (posicionActual > 0) {
                     int diferencia = abs(numeroIntento - arregloNumeros[posicionActual - 1]);
                     diferenciasUsadas[diferencia] = 0;
                 }
                 numerosUsados[numeroIntento] = 0;
             }
         }
     }
 }
 
 /**
  * @brief Función principal del programa.
  *
  * @param argc Número de argumentos de línea de comandos.
  * @param argv Argumentos de línea de comandos.
  * @return int Código de salida del programa.
  */
 int main(int argc, char *argv[]) {
     // Verificar que el usuario ingreso los argumentos necesarios
     if (argc != 3) {
         printf("Uso: %s <N> <M>\n", argv[0]);
         return 1;
     }
     
     int N = atoi(argv[1]);  // Convertir argumento a entero
     int M = atoi(argv[2]);  // Convertir argumento a entero
 
     // Validar que los valores ingresados sean correctos
     if (N <= 1 || N >= 50 || M <= 0) {
         printf("El numero debe estar en 1 < N < 50 y el tiempo maximo debe ser positivo\n");
         return 1;
     }
     
     // Declaracion de arreglos con el tamaño ingresado por el usuario
     int arregloNumeros[N];  
     int numerosUsados[N + 1];  
     int diferenciasUsadas[N];  
 
     // Inicializar arreglos con ceros
     for (int i = 0; i <= N; i++) {
         numerosUsados[i] = 0;
     }
     for (int i = 0; i < N; i++) {
         diferenciasUsadas[i] = 0;
     }
 
     unsigned long totalPermutaciones = 0;
     clock_t tiempoInicio = clock();  // Guardar el tiempo de inicio
 
     encontrarPermutaciones(0, N, arregloNumeros, numerosUsados, diferenciasUsadas, &totalPermutaciones, tiempoInicio, M);
 
     double tiempoTotal = (double)(clock() - tiempoInicio) / CLOCKS_PER_SEC;
     
     // Mostrar resultados
     printf("Cantidad de numeros ingresada: %d\n", N);
     if (tiempoTotal >= M * 60) {
         printf("[AVISO] No se pudo terminar de permutar en el tiempo limite.\n");
     }
     printf("Numero total de permutaciones graciles: %ld\n", totalPermutaciones);
     printf("Tiempo total de ejecucion: %.6f segundos\n", tiempoTotal);
     
     return 0;
 }
 
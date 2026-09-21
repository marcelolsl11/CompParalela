/*
 * soma_vetores.c
 * Parte 2 - Opcao C: Soma de vetores gigantes C[i] = A[i] + B[i].
 *
 * Uso: ./soma_vetores <N> <num_threads>
 *   ex.: ./soma_vetores 50000000 4
 *
 * A operacao e O(1) por elemento, mas o problema e MEMORY-BOUND: exige
 * transferir 3 vetores enormes de/para a RAM. Dividimos o vetor em blocos
 * CONTIGUOS de indices entre as threads para favorecer localidade espacial
 * de cache. Nao ha escrita compartilhada (cada thread escreve sua faixa de
 * C), logo nao e preciso mutex.
 *
 * O Speedup satura cedo porque a largura de banda do barramento de memoria
 * (DRAM bandwidth) vira o gargalo, nao a CPU.
 *
 * Compilacao: gcc -O2 -Wall soma_vetores.c -o soma_vetores -pthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    const double* A;
    const double* B;
    double* C;
    long inicio;  /* indice inicial (inclusivo) */
    long fim;     /* indice final (exclusivo) */
} ArgThread;

static void* trabalho(void* arg) {
    ArgThread* a = (ArgThread*)arg;
    for (long i = a->inicio; i < a->fim; i++) {
        a->C[i] = a->A[i] + a->B[i];
    }
    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <N> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long N = strtol(argv[1], NULL, 10);
    int num_threads = atoi(argv[2]);
    if (N < 1 || num_threads < 1) {
        fprintf(stderr, "Parametros invalidos: N>=1 e num_threads>=1\n");
        return EXIT_FAILURE;
    }

    double* A = malloc(sizeof(double) * N);
    double* B = malloc(sizeof(double) * N);
    double* C = malloc(sizeof(double) * N);
    if (!A || !B || !C) {
        fprintf(stderr, "Falha de alocacao (N muito grande?)\n");
        free(A); free(B); free(C);
        return EXIT_FAILURE;
    }

    /* Inicializacao dos vetores (fase sequencial). */
    for (long i = 0; i < N; i++) {
        A[i] = (double)i * 0.5;
        B[i] = (double)i * 1.5;
    }

    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    ArgThread* args = malloc(sizeof(ArgThread) * num_threads);
    if (!threads || !args) {
        fprintf(stderr, "Falha de alocacao\n");
        free(A); free(B); free(C); free(threads); free(args);
        return EXIT_FAILURE;
    }

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    /* Particao por blocos contiguos. */
    long base = N / num_threads;
    long resto = N % num_threads;
    long corrente = 0;

    for (long t = 0; t < num_threads; t++) {
        long tamanho = base + (t < resto ? 1 : 0);
        args[t].A = A;
        args[t].B = B;
        args[t].C = C;
        args[t].inicio = corrente;
        args[t].fim = corrente + tamanho;
        corrente += tamanho;
        pthread_create(&threads[t], NULL, trabalho, &args[t]);
    }

    for (long t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) +
                   (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    /* Verificacao de corretude em uma amostra. */
    long idx = N / 2;
    double esperado = A[idx] + B[idx];

    printf("[SomaVetores] N=%ld | threads=%d | C[%ld]=%.2f (esperado %.2f) | Tempo: %.4f s\n",
           N, num_threads, idx, C[idx], esperado, tempo);

    free(A); free(B); free(C);
    free(threads); free(args);
    return 0;
}

/*
 * monte_carlo_pi.c
 * Parte 2 - Opcao B: Estimativa de pi pelo metodo de Monte Carlo.
 *
 * Sorteia N pontos (x,y) em [0,1]x[0,1] e conta quantos caem no quadrante
 * de circulo (x^2 + y^2 <= 1). pi ~= 4 * (dentro / N).
 *
 * Desafio de thread-safety: rand() usa estado global protegido por lock
 * na glibc, o que SERIALIZA as threads e anula o ganho paralelo. Usamos
 * rand_r(&seed), reentrante, com uma SEMENTE INDEPENDENTE por thread.
 * Cada thread acumula localmente e reduz uma vez ao final.
 *
 * Compilacao: gcc -O2 -Wall monte_carlo_pi.c -o monte_carlo_pi -pthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct {
    long pontos;        /* quantidade de pontos que esta thread deve sortear */
    unsigned int seed;  /* semente privada para rand_r */
    long dentro;        /* saida: pontos que cairam no circulo */
} ArgThread;

static void* trabalho(void* arg) {
    ArgThread* a = (ArgThread*)arg;
    long dentro = 0;
    unsigned int seed = a->seed;  /* copia local do estado do RNG */

    for (long i = 0; i < a->pontos; i++) {
        double x = (double)rand_r(&seed) / (double)RAND_MAX;
        double y = (double)rand_r(&seed) / (double)RAND_MAX;
        if (x * x + y * y <= 1.0) {
            dentro++;
        }
    }

    a->dentro = dentro;
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

    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    ArgThread* args = malloc(sizeof(ArgThread) * num_threads);
    if (!threads || !args) {
        fprintf(stderr, "Falha de alocacao\n");
        free(threads); free(args);
        return EXIT_FAILURE;
    }

    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    /* Distribui N pontos entre as threads; a ultima absorve o resto. */
    long base = N / num_threads;
    long resto = N % num_threads;

    for (long t = 0; t < num_threads; t++) {
        args[t].pontos = base + (t == num_threads - 1 ? resto : 0);
        /* Semente distinta por thread para independencia estatistica. */
        args[t].seed = (unsigned int)(time(NULL) ^ (t * 2654435761u + 1u));
        args[t].dentro = 0;
        pthread_create(&threads[t], NULL, trabalho, &args[t]);
    }

    long total_dentro = 0;
    for (long t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
        total_dentro += args[t].dentro;  /* reducao via retorno por join */
    }

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) +
                   (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    double pi = 4.0 * (double)total_dentro / (double)N;

    printf("[MonteCarlo] N=%ld | threads=%d | pi~=%.8f | Tempo: %.4f s\n",
           N, num_threads, pi, tempo);

    free(threads);
    free(args);
    return 0;
}


/*
 * primos.c
 * Parte 2 - Opcao A: Contagem de numeros primos no intervalo [1, K].
 *
 * Uso: ./primos <K> <num_threads>
 *   ex.: ./primos 5000000 4
 *
 * Desafio de balanceamento de carga: testar a primalidade de numeros
 * grandes custa mais que de numeros pequenos. Uma particao estatica por
 * blocos contiguos deixaria as ultimas threads sobrecarregadas. Por isso
 * usamos DISTRIBUICAO CICLICA (stride): a thread t testa os numeros
 * t, t+P, t+2P, ..., o que intercala numeros pequenos e grandes entre
 * todas as threads e equilibra a carga.
 *
 * Cada thread acumula uma contagem LOCAL e soma ao total global uma unica
 * vez ao final, protegida por mutex (reducao), evitando contencao por
 * iteracao.
 *
 * Compilacao: gcc -O2 -Wall primos.c -o primos -pthread -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

typedef struct {
    long id;          /* indice da thread (0..num_threads-1) */
    long K;           /* limite superior do intervalo */
    int num_threads;  /* total de threads */
    long parcial;     /* saida: primos contados por esta thread */
} ArgThread;

static long total_primos = 0;
static pthread_mutex_t mutex_total = PTHREAD_MUTEX_INITIALIZER;

/* Teste de primalidade classico O(sqrt(n)). */
static int eh_primo(long n) {
    if (n < 2) return 0;
    if (n < 4) return 1;          /* 2 e 3 */
    if (n % 2 == 0) return 0;
    long limite = (long)sqrt((double)n);
    for (long d = 3; d <= limite; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

static void* trabalho(void* arg) {
    ArgThread* a = (ArgThread*)arg;
    long local = 0;

    /* Distribuicao ciclica: comeca em (id+1) e pula de num_threads em
       num_threads. Ex.: thread 0 -> 1, 1+P, 1+2P ... */
    for (long n = a->id + 1; n <= a->K; n += a->num_threads) {
        if (eh_primo(n)) {
            local++;
        }
    }

    a->parcial = local;

    /* Reducao unica no acumulador global. */
    pthread_mutex_lock(&mutex_total);
    total_primos += local;
    pthread_mutex_unlock(&mutex_total);

    return NULL;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <K> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long K = strtol(argv[1], NULL, 10);
    int num_threads = atoi(argv[2]);
    if (K < 1 || num_threads < 1) {
        fprintf(stderr, "Parametros invalidos: K>=1 e num_threads>=1\n");
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

    for (long t = 0; t < num_threads; t++) {
        args[t].id = t;
        args[t].K = K;
        args[t].num_threads = num_threads;
        args[t].parcial = 0;
        pthread_create(&threads[t], NULL, trabalho, &args[t]);
    }

    for (long t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) +
                   (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("[Primos] K=%ld | threads=%d | primos=%ld | Tempo: %.4f s\n",
           K, num_threads, total_primos, tempo);

    pthread_mutex_destroy(&mutex_total);
    free(threads);
    free(args);
    return 0;
}

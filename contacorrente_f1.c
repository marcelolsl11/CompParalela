/*
 * contacorrente_f1.c
 * Parte 1 - Fase 1: O caos concorrente (condicao de corrida).
 *
 * Duas threads manipulam a variavel global 'saldo' simultaneamente, sem
 * qualquer sincronizacao. Como 'saldo += x' nao e atomico (le, soma,
 * escreve), ocorrem lost updates: o resultado final e nao deterministico.
 * Execute varias vezes e observe que o saldo muda a cada execucao.
 *
 * Compilacao: gcc -O2 -Wall contacorrente_f1.c -o contacorrente_f1 -pthread
 */
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define NUM_OPERACOES 50000000
#define VALOR_DEPOSITO 5.0
#define VALOR_SAQUE 2.0

static double saldo = 1000.00;

static void* thread_depositos(void* arg) {
    (void)arg;
    for (long i = 0; i < NUM_OPERACOES; i++) {
        saldo += VALOR_DEPOSITO; /* regiao critica desprotegida */
    }
    return NULL;
}

static void* thread_saques(void* arg) {
    (void)arg;
    for (long i = 0; i < NUM_OPERACOES; i++) {
        saldo -= VALOR_SAQUE; /* regiao critica desprotegida */
    }
    return NULL;
}

int main(void) {
    pthread_t t_dep, t_saq;
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    pthread_create(&t_dep, NULL, thread_depositos, NULL);
    pthread_create(&t_saq, NULL, thread_saques, NULL);

    pthread_join(t_dep, NULL);
    pthread_join(t_saq, NULL);

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) +
                   (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("[Fase 1 - Race Condition] Saldo final: %.2f | Tempo: %.4f s\n",
           saldo, tempo);
    return 0;
}

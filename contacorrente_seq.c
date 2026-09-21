/*
 * contacorrente_seq.c
 * Parte 1 - Fase 0: Baseline sequencial.
 *
 * As operacoes de deposito e saque sao executadas em serie na thread
 * principal. Serve como referencia de corretude (saldo deterministico) e
 * de tempo para comparacao com as versoes concorrentes.
 *
 * Compilacao: gcc -O2 -Wall contacorrente_seq.c -o contacorrente_seq
 */
#include <stdio.h>
#include <time.h>

#define NUM_OPERACOES 50000000
#define VALOR_DEPOSITO 5.0
#define VALOR_SAQUE 2.0

/* Saldo compartilhado (aqui sem concorrencia, apenas referencia). */
static double saldo = 1000.00;

static void depositos(void) {
    for (long i = 0; i < NUM_OPERACOES; i++) {
        saldo += VALOR_DEPOSITO;
    }
}

static void saques(void) {
    for (long i = 0; i < NUM_OPERACOES; i++) {
        saldo -= VALOR_SAQUE;
    }
}

int main(void) {
    struct timespec inicio, fim;
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    depositos();
    saques();

    clock_gettime(CLOCK_MONOTONIC, &fim);
    double tempo = (fim.tv_sec - inicio.tv_sec) +
                   (fim.tv_nsec - inicio.tv_nsec) / 1e9;

    printf("[Sequencial] Saldo final: %.2f | Tempo: %.4f s\n", saldo, tempo);
    return 0;
}

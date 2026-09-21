"""
seq.py
Parte 3 - Codigo 1: Trabalho CPU-bound executado sequencialmente.

Executa duas contagens decrescentes pesadas em serie. Serve de baseline
para comparar com a versao com threads (threads.py) e evidenciar o efeito
do GIL do CPython.
"""
import time


def contagem_pesada(n):
    while n > 0:
        n -= 1


if __name__ == "__main__":
    n = 100_000_000
    inicio = time.time()

    # Executa a contagem pesada duas vezes sequencialmente.
    contagem_pesada(n)
    contagem_pesada(n)

    fim = time.time()
    print(f"[Python Sequencial] Tempo: {fim - inicio:.4f} segundos")

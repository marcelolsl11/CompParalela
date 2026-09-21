"""
threads.py
Parte 3 - Codigo 2: Mesmo trabalho CPU-bound com duas threads.

Cria duas threads, cada uma executando uma contagem pesada. Em teoria
deveria ser ~2x mais rapido em uma maquina multi-core, mas por causa do
GIL (Global Interpreter Lock) do CPython apenas uma thread executa
bytecode por vez. O tempo fica igual ou ate PIOR que o sequencial, pois
soma-se o overhead de troca de contexto entre as threads.
"""
import time
import threading


def contagem_pesada(n):
    while n > 0:
        n -= 1


if __name__ == "__main__":
    n = 100_000_000
    inicio = time.time()

    # Cada thread executa uma contagem pesada.
    t1 = threading.Thread(target=contagem_pesada, args=(n,))
    t2 = threading.Thread(target=contagem_pesada, args=(n,))

    t1.start()
    t2.start()

    t1.join()
    t2.join()

    fim = time.time()
    print(f"[Python com Threads] Tempo: {fim - inicio:.4f} segundos")

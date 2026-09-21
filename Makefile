# Makefile - Laboratorio Pthreads (CompPar)
# Compila os programas em C (Partes 1 e 2) e oferece alvos de execucao
# para reproduzir todas as medicoes do relatorio.
#
# Uso principal:
#   make              -> compila todos os binarios
#   make clean        -> remove binarios
#   make run-parte1   -> executa Fase 0, 5x a Fase 1 e a Fase 2
#   make run-parte2   -> executa primos, monte_carlo_pi e soma_vetores (1/2/4/8 threads)
#   make run-parte3   -> executa seq.py e threads.py (Parte 3 - GIL)
#   make bench        -> executa tudo (Partes 1, 2 e 3) em sequencia
#   make relatorio    -> gera ../RELATORIO.pdf a partir de ../RELATORIO.md (pandoc)

CC      = gcc
CFLAGS  = -O2 -Wall -pthread
LDLIBS  = -lm
PYTHON  = python3

# Parametros das medicoes (ajuste conforme sua maquina)
K_PRIMOS   = 5000000
N_PI       = 100000000
N_VETORES  = 50000000
THREADS    = 1 2 4 8

BINARIOS = contacorrente_seq contacorrente_f1 contacorrente_f2 \
           primos monte_carlo_pi soma_vetores

all: $(BINARIOS)

# ---------- Parte 1 ----------
contacorrente_seq: contacorrente_seq.c
	$(CC) $(CFLAGS) $< -o $@

contacorrente_f1: contacorrente_f1.c
	$(CC) $(CFLAGS) $< -o $@

contacorrente_f2: contacorrente_f2.c
	$(CC) $(CFLAGS) $< -o $@

# ---------- Parte 2 (primos usa a libm por causa de sqrt) ----------
primos: primos.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

monte_carlo_pi: monte_carlo_pi.c
	$(CC) $(CFLAGS) $< -o $@

soma_vetores: soma_vetores.c
	$(CC) $(CFLAGS) $< -o $@

# ---------- Alvos de execucao ----------
run-parte1: contacorrente_seq contacorrente_f1 contacorrente_f2
	@echo "== Fase 0 - Sequencial =="
	./contacorrente_seq
	@echo "== Fase 1 - Race Condition (5 execucoes) =="
	@for i in 1 2 3 4 5; do ./contacorrente_f1; done
	@echo "== Fase 2 - Mutex =="
	./contacorrente_f2

run-parte2: primos monte_carlo_pi soma_vetores
	@echo "== A - Primos (K=$(K_PRIMOS)) =="
	@for t in $(THREADS); do ./primos $(K_PRIMOS) $$t; done
	@echo "== B - Monte Carlo (N=$(N_PI)) =="
	@for t in $(THREADS); do ./monte_carlo_pi $(N_PI) $$t; done
	@echo "== C - Soma de Vetores (N=$(N_VETORES)) =="
	@for t in $(THREADS); do ./soma_vetores $(N_VETORES) $$t; done

run-parte3:
	@echo "== Python Sequencial =="
	$(PYTHON) seq.py
	@echo "== Python com Threads (GIL) =="
	$(PYTHON) threads.py

bench: run-parte1 run-parte2 run-parte3

# ---------- Geracao do PDF do relatorio ----------
relatorio: ../RELATORIO.pdf

../RELATORIO.pdf: ../RELATORIO.md
	cd .. && pandoc RELATORIO.md -o RELATORIO.pdf \
	    --pdf-engine=lualatex -V geometry:margin=2.3cm \
	    -V lang=pt-BR -V fontsize=11pt -V colorlinks=true --toc

clean:
	rm -f $(BINARIOS)

.PHONY: all clean run-parte1 run-parte2 run-parte3 bench relatorio

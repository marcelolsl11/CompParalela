# Relatório de Laboratório - Computação Paralela

**Disciplina:** Computação Paralela
**FCI - Universidade Presbiteriana Mackenzie**
**Integrante:** Marcelo Luis Simone Lucas - RA: 10332213
**Repositório:** https://github.com/marcelolsl11/CompParalela
**Data de submissão:** 21/09/2026

## Resumo

Este trabalho investiga na prática os fundamentos da programação paralela em memória compartilhada usando a biblioteca POSIX Threads (Pthreads) em C. Também avalia a restrição de concorrência imposta pelo Global Interpreter Lock (GIL) no CPython. Reproduzi uma clássica condição de corrida no problema da conta corrente. A correção foi feita aplicando exclusão mútua. Implementei três problemas paralelos numéricos: contagem de primos, estimativa de π via Monte Carlo e soma de vetores gigantes. Neles, medi a escalabilidade para 1, 2, 4 e 8 threads. Por fim, comparei as threads nativas do C com as do Python em uma tarefa dependente de CPU. Os resultados mostram o custo-benefício entre consistência e sobrecarga de sincronização. Eles também expõem os limites físicos da largura de banda de memória e a serialização forçada pelo GIL.

## 1. Introdução e Objetivos

O paralelismo em memória compartilhada domina os processadores multi-core modernos. Nesse modelo, várias threads de um mesmo processo dividem o espaço de endereçamento. Isso gera eficiência na comunicação. No entanto, introduz o risco constante de acessos concorrentes inconsistentes.

Meus objetivos neste laboratório foram:

1. Compreender o ciclo de vida das threads usando `pthread_create` e `pthread_join`.

2. Diagnosticar os efeitos das condições de corrida sobre variáveis compartilhadas.

3. Implementar regiões críticas com `pthread_mutex_t` e mensurar seu custo computacional.

4. Aplicar padrões de divisão de trabalho (worksharing) em problemas numéricos. O foco foi medir Speedup e Eficiência escalando entre 1 e 8 threads.

5. Comparar o modelo de threads do C com o do CPython. A meta era entender o impacto real do GIL em tarefas CPU-bound.

## 2. Arquitetura, Metodologia e Ambiente Experimental

### 2.1 Organização do código

Os arquivos fonte estão no diretório `src/`:

| 

| **Arquivo** | **Parte** | **Descrição** | 
| `contacorrente_seq.c` | 1 - Fase 0 | Base sequencial (baseline) | 
| `contacorrente_f1.c` | 1 - Fase 1 | Concorrente sem sincronização (condição de corrida) | 
| `contacorrente_f2.c` | 1 - Fase 2 | Concorrente com exclusão mútua (mutex) | 
| `primos.c` | 2 - A | Contagem de primos (distribuição cíclica + redução) | 
| `monte_carlo_pi.c` | 2 - B | Estimativa de π com a função reentrante `rand_r` | 
| `soma_vetores.c` | 2 - C | Soma de vetores (blocos contíguos, cenário memory-bound) | 
| `seq.py` / `threads.py` | 3 | Comparação entre execução sequencial e threads (efeito do GIL) | 
| `Makefile` | \- | Automação da compilação de todos os arquivos `.c` | 

### 2.2 Compilação

```
cd src
make            # compila todos os binários

```

O `Makefile` utiliza as flags `gcc -O2 -Wall -pthread`. O arquivo `primos.c` inclui `-lm` devido ao uso da função `sqrt`. A compilação ocorreu de forma **limpa e sem advertências (warnings)** em todos os seis programas, conforme a Figura 1. O Makefile oferece atalhos práticos para os testes: `make run-parte1`, `make run-parte2`, `make run-parte3` e `make bench`.

### 2.3 Metodologia de medição

Usei a função `clock_gettime(CLOCK_MONOTONIC, ...)` para todas as medições de tempo. Essa abordagem blinda os resultados contra eventuais ajustes do relógio do sistema. Calculei o Speedup com a fórmula `Sp = T1 / Tp`. A Eficiência foi extraída por `Ep = Sp / p`, sendo `p` a contagem de threads. As Figuras 1 a 5 são **capturas de tela reais do terminal** geradas durante as baterias de teste. Os números exibidos nessas imagens alimentam exatamente as tabelas deste documento.

### 2.4 Ambiente experimental

> **Nota sobre a infraestrutura:** Compilei e rodei os testes no **WSL2 (Windows Subsystem for Linux)** usando o Ubuntu 24.04 LTS. A virtualização leve do WSL2 pode alterar marginalmente os tempos absolutos. As tendências relativas de Speedup e saturação permanecem precisas.

| **Componente** | **Especificação** | 
| Processador | 12th Gen Intel Core i5-1235U | 
| Núcleos | 6 núcleos físicos, 2 threads por núcleo. Total: 12 threads lógicas | 
| Cache | L1d 288 KiB (6x) / L1i 192 KiB (6x) / L2 7,5 MiB (6x) / L3 12 MiB | 
| Memória RAM | \~8 GB (7.992.520 kB) alocados para o WSL2 | 
| Sistema Operacional | Ubuntu 24.04.3 LTS - kernel 6.6.87.2-microsoft-standard-WSL2 | 
| Compilador | gcc (Ubuntu 13.3.0-6ubuntu2\~24.04.1) versão 13.3.0 | 
| Interpretador | Python 3.12.3 | 
| Flags de compilação | `-O2 -Wall -pthread` (além de `-lm` para `primos.c`) | 

> **Nota sobre o processador i5-1235U:** Trata-se de um chip de arquitetura híbrida. Ele combina 2 núcleos de performance (P-cores com Hyper-Threading) e 4 núcleos de eficiência (E-cores). Isso justifica a ausência de um Speedup perfeitamente linear até 8 threads. Threads adicionais acabam mapeadas nos E-cores, que entregam menor rendimento.

## 3. Implementação e Resultados

### 3.1 Parte 1 - Conta Corrente

O saldo matemático esperado neste cenário é fixo:

```
Saldo Final = 1000 + (50.000.000 × 5,00) − (50.000.000 × 2,00) = 150.001.000,00

```

* **Fase 0 (sequencial):** Retornou exatos `150.001.000,00` em 0,0473 s. Isso estabeleceu a linha de base de corretude e tempo.

* **Fase 1 (condição de corrida):** A operação `saldo += x` não é atômica. As threads sobrepõem as atualizações uma da outra. Após 5 execuções, o saldo final se mostrou **completamente instável**. Quatro testes cravaram 250.001.000,00. O quinto marcou −99.999.000,00. O valor correto nunca foi atingido.

* **Fase 2 (mutex):** O controle de exclusão mútua restaurou o resultado exato de `150.001.000,00`. O problema foi o tempo de execução. Ele saltou para 5,0526 s. O processo ficou **107 vezes mais lento** que o sequencial. Esse é o peso de travar e destravar o sistema 100 milhões de vezes.

**Tabela 1 - Conta Corrente (tempos reais medidos):**

| **Versão** | **Saldo final** | **Tempo (s)** | 
| Fase 0 - Sequencial | 150.001.000,00 (correto) | 0,0473 | 
| Fase 1 - Execução 1 | 250.001.000,00 | 0,0242 | 
| Fase 1 - Execução 2 | 250.001.000,00 | 0,0267 | 
| Fase 1 - Execução 3 | 250.001.000,00 | 0,0265 | 
| Fase 1 - Execução 4 | −99.999.000,00 | 0,0271 | 
| Fase 1 - Execução 5 | 250.001.000,00 | 0,0270 | 
| Fase 2 - Mutex | 150.001.000,00 (correto) | 5,0526 | 

A Fase 1 corrompe os dados de forma imprevisível. A Fase 2 garante a matemática correta, mas impõe um pedágio insustentável de performance.

### 3.2 Parte 2 - Menu (opções escolhidas: A, B e C)

Selecionei os problemas **A (primos)**, **B (Monte Carlo π)** e **C (soma de vetores)**. Eles expõem três realidades distintas. Temos carga CPU-bound com risco de desbalanceamento, a questão do thread-safety em geradores aleatórios e a barreira de hardware em operações memory-bound.

**Estratégias de particionamento adotadas:**

* **A - Primos:** Usei a distribuição **cíclica**. A thread `t` avalia os índices `t+1, t+1+P, t+1+2P, ...`. Isso mistura números pequenos e grandes, distribuindo a carga matemática pesada por igual. Cada thread soma localmente e faz uma **redução única** sob mutex no fim do processo.

* **B - Monte Carlo:** Apliquei o particionamento estático dos pontos. Cada thread ganha uma **semente própria** alimentando a função reentrante `rand_r(&seed)`. A redução acontece naturalmente com o `pthread_join`.

* **C - Soma de vetores:** Dividi o processamento em **blocos contíguos** de índices. Isso maximiza a localidade do cache espacial. Como não há escrita em dados compartilhados, dispensei o mutex completamente.

**Tabela 2 - Escalabilidade (valores medidos, S = Speedup = T1/Tp, E = Eficiência = S/p):**

| **Problema** | **Parâmetro** | **1 thread (s)** | **2 threads (s)** | **4 threads (s)** | **8 threads (s)** | **S₂** | **S₄** | **S₈** | **E₈** | 
| A - Primos | K=5.000.000 | 0,7447 | 0,7583 | 0,4127 | 0,2297 | 0,98 | 1,80 | 3,24 | 0,41 | 
| B - Monte Carlo | N=100.000.000 | 0,6025 | 0,3253 | 0,1777 | 0,0948 | 1,85 | 3,39 | 6,36 | 0,79 | 
| C - Soma vetores | N=50.000.000 | 0,2204 | 0,0792 | 0,0602 | 0,0504 | 2,78 | 3,66 | 4,37 | 0,55 | 

Os três algoritmos operaram com exatidão matemática. A contagem isolou **348.513** números primos. O Monte Carlo entregou π ≈ 3,1416. A soma vetorizada atestou `C[N/2] = 50.000.000,00` em todos os testes.

**Análise dos resultados obtidos:**

* **A - Primos:** O tempo cravou entre 0,74 s e 0,76 s na passagem de 1 para 2 threads. O S₂ ficou em 0,98. O trabalho de criar a thread extra cobrou mais tempo do que a execução em si. Provavelmente as duas lógicas caíram no mesmo P-core físico do processador Intel. A escalabilidade real só acordou com 8 threads (S₈ = 3,24).

* **B - Monte Carlo:** Apresentou a **melhor escalabilidade** do grupo. Chegou a S₈ = 6,36 com 79% de eficiência. O processo é isolado na CPU e não requer comunicação externa. O `rand_r` blindou o estado do gerador aleatório, evitando qualquer gargalo no interpretador.

* **C - Soma de vetores:** O comportamento memory-bound é brutal. Tivemos **superspeedup** na transição inicial (S₂ = 2,78). Com duas threads dividindo a carga, os caches L2 processaram a informação de forma otimizada. A curva despenca logo em seguida. Entre 4 e 8 threads, o tempo cai de 0,0602 s para parcos 0,0504 s. A eficiência derrete de 0,92 para 0,55. O limite físico da largura de banda da memória RAM (DRAM) freia a operação de forma abrupta.

**Gráficos de desempenho:**

### 3.3 Cálculo de Speedup e Eficiência

Matematicamente, as métricas respondem a:

```
Sp = T1 / Tp        (aceleração)
Ep = Sp / p         (eficiência, cujo limite ideal é 1,0)

```

A Tabela 2 apresenta as consolidações numéricas. Nas Figuras 6 e 7 temos as representações gráficas. A linha tracejada na Figura 6 projeta o paralelismo perfeito (`y = x`). O descolamento das linhas reais em relação a essa reta ilustra a eficiência perdida na prática. Monte Carlo raspa na perfeição teórica. A busca por primos sofre a maior penalização lógica.

### 3.4 Parte 3 - Python e o GIL

Rodei os scripts `seq.py` e `threads.py` utilizando o escopo de `n = 100.000.000` nativo do Python 3.12.3:

| **Versão** | **Tempo (s)** | **Trabalho total** | 
| Sequencial (duas contagens em série) | **2,7810** | 2 × 100M decrementos | 
| Com 2 threads | **3,0119** | 2 × 100M decrementos | 

A conclusão é clara e problemática. Ambas as versões bateram 200 milhões de decrementos lógicos. A rotina paralela foi, no entanto, **mais lenta** que a estrutura sequencial engessada. O paralelismo autêntico deveria dividir o tempo pela metade (\~1,4 s). A arquitetura Python degradou a velocidade em cerca de 8%.

A culpa é do Global Interpreter Lock (GIL). Essa trava de sistema autoriza o bytecode Python em apenas uma thread por vez. As threads brincam de revezamento no processador, achatando o paralelismo a zero. A versão paralela carrega ainda o fardo computacional de instanciar e negociar o lock repetidas vezes. O cenário no C é oposto. A mesma matemática intensiva escalou em 6,36x simplesmente porque não há amarras globais de interpretador no compilador nativo.

## 4. Respostas às 9 Questões de Reflexão

**1. Ciclo leitura-modificação-escrita (Fase 1).** O comando `saldo += VALOR_DEPOSITO` não detém atomicidade em hardware. A máquina executa três ações avulsas: carrega o dado da memória, soma no registrador e grava o resultado. Duas threads agindo simultaneamente puxam o mesmo saldo base obsoleto. A thread mais lenta sobrescreve o resultado da thread mais rápida na gravação final. Como a memória cache e os registradores são privados de cada núcleo, não há consistência. O resultado corrompe a cada tentativa.

**2. Impacto e granularidade do mutex (Fase 2).** O tipo `pthread_mutex_t` serializa o acesso forçado. Uma única thread adquire o lock, mexe no saldo e libera a trava, blindando a matemática. O problema é a escala dessa trava. Ela ocorre **dentro** de um looping acionado 100 milhões de vezes. Operações de travamento demandam chamadas diretas ao escalonador do SO, gerando trocas de contexto onerosas. A Fase 2 gastou 5,0526 s para resolver o que a Fase 0 fez em 0,0473 s. Injetar sincronização cega em granularidade fina massacra a performance geral.

**3. Estratégias alternativas de sincronização.** A) **Redução em variáveis isoladas:** Cada núcleo acumula somas internamente sem uso de travas sistêmicas. Ao fim da tarefa, as threads somam seus blocos ao montante global. Isso derruba as ativações do lock de 100 milhões para uma única por processo. B) **Atômicos C11 (`<stdatomic.h>`):** Configurar variáveis em `atomic_long` permite explorar a instrução `atomic_fetch_add`. Isso une a leitura e gravação num pulso de hardware indivisível. Elimina o tranco do mutex, mas cobra tributo na coerência de dados em cache no acesso agressivo.

**4. Decomposição e balanceamento de carga (Parte 2).** Apliquei fatiamento cíclico para os primos e blocos estáticos nas demais rotinas. Testes de primalidade exigem poder computacional atrelado a `sqrt(n)`. Agrupar os números de forma linear estrangularia o núcleo escalado para a porção final da tabela numérica. A cadência cíclica divide uniformemente números fáceis e difíceis. Desbalancear a carga detona a eficiência porque a máquina inteira aguarda ocosamente a thread mais afogada terminar.

**5. Reentrância e thread-safety (Opção B).** A função `rand()` se escora em estados matemáticos retidos globalmente. A biblioteca glibc tranca esses estados na marra para barrar corrupções. Isso transforma execuções paralelas em filas indianas sequenciais. Funções reentrantes quebram esse ciclo porque operam alheias aos estados externos estáticos. Usar `rand_r(unsigned int *seed)` empurra a semente geradora direto no ponteiro local. O compartilhamento some. O processamento escala livremente.

**6. Escalabilidade e limites de memória (Opções C/E).** Rotinas com perfil memory-bound vão inevitavelmente colidir com a realidade física das placas. A carga matemática nesses sistemas é mínima se comparada à fome por transporte de dados na DRAM. A eficiência calculada caiu de 1,39 para sombrios 0,55 com apenas 8 núcleos ativos. Barramentos de memória não esticam. Empilhar threads satura a banda física, sufocando os ganhos progressivamente. Isso distancia o cenário das projeções puramente CPU-bound.

**7. O papel do GIL no CPython (Parte 3).** O GIL existe para proteger a arquitetura de gerenciamento e contagem de referências do Python contra anomalias concorrentes. Sem ele, o interpretador precisaria entupir as variáveis básicas com proteções atômicas intrusivas. Isso faria programas em thread única rodarem de forma abissal. O preço de mercado dessa escolha é a interdição severa de algoritmos CPU-bound. As threads lutam incessantemente pelo direito de interpretação, torrando desempenho real em burocracia de sistema.

**8. Paralelismo real em Python (Parte 3).** A única saída pragmática passa pelo pacote `multiprocessing`. Ele isola processos independentes direto na raiz do SO, armando cada núcleo com seu próprio interpretador e seu próprio GIL. O entrave se torna logístico. Disparar processos isolados torra memória de forma insana. Além disso, a troca de contexto entre blocos carece de pontes pesadas de serialização e IPC (como o `pickle`).

**9. Lei de Amdahl.** A Lei de Amdahl comprova que qualquer aceleração em larga escala morre afogada na fração sequencial inevitável do código (as criações estruturais de memória e consolidação via `pthread_join`).

```
S(p) = 1 / ( f + (1 − f)/p )      e, com p → ∞,      S_max = 1/f

```

Nos ensaios otimizados do Monte Carlo, o sistema atingiu um viés de 3,7% de tempo amarrado em rotinas sequenciais (f ≈ 0,037). Isso indica um teto implacável em `27x` de ganho, independente da contagem de processadores instalada no cluster. Vale pontuar que a matemática de Amdahl supõe hardware de CPU livre. Ela peca redondamente ao projetar métricas memory-bound, que operam presas na asfixia direta das frequências de barramento da placa mãe.

## 5. Dificuldades Técnicas e Soluções

A implementação empírica trouxe as seguintes barreiras:

* **Alocação bruta de variáveis via `void*`:** Compartilhar contadores fixos pelo laço de instância resultava em sobreposição severa nas threads ativadas. Desenhei a `struct ArgThread` dedicada e injetei via apontamento isolado para blindar os dados.

* **Tráfego paralisante do mutex:** A Fase 2 revelou a fraqueza prática das travas finas. Registrei conceitualmente como contornar isso escalando atômicos e retenções assíncronas.

* **Isolamento em Monte Carlo:** Os resultados estáticos iniciais com `rand()` desvendaram gargalos no núcleo da glibc. Migrar a rotina limpa para a entrada encapsulada `rand_r` sanou os retardos.

* **Distribuição viciada nos Primos:** As divisões em blocos simétricos enforcaram sistematicamente os núcleos alocados na régua alta. Apliquei lógicas intermitentes de fatiamento cíclico para diluir a barreira.

* **Choque na banda da RAM:** A mortandade progressiva da aceleração na escala vetorial atestou a brutalidade física das placas DRAM frente ao volume numérico imposto.

* **Engenharia de entrega:** Amarrei todo o fluxo de relatório e gráficos no ecossistema Makefile. Integrações automatizadas com `pandoc` e matplotlib viabilizaram a geração ágil da base para este material.

## 6. Declaração e Análise do Uso de Ferramentas de IA Generativa

### 6.1 Registro transparente do uso de IA

Acessei suportes de Inteligência Artificial para estruturação rápida das mecânicas fundamentais e abstrações semânticas da base, centralizados em:

* Montagem expressa de templates e marcações rasas ligadas ao pareamento entre variáveis da struct `ArgThread` e os ponteiros voláteis.

* Exploração rápida das limitações enraizadas de lock global escondido na variante original `rand()`.

* Diagnóstico sintático perante avisos ríspidos emitidos pelo gcc no trato da concorrência de memórias em vetores longos.

### 6.2 Tabela de prompts formulados e escopo de aplicação

| **Etapa do Laboratório** | **Prompt Formulado** | **Objetivo e Parte Atendida** | 
| Modelagem inicial | *"Como passar múltiplos argumentos para uma thread Pthreads usando struct e ponteiro void?"* | Esqueleto de passagem de parâmetros (Parte 2) | 
| Conceituação teórica | *"Por que rand() serializa threads na glibc e como rand_r resolve isso?"* | Fundamentação da Opção B e Questão 5 | 
| Balanceamento de carga | *"Distribuição cíclica vs blocos contíguos na contagem de primos - qual equilibra melhor?"* | Estratégia da Opção A e Questão 4 | 
| Depuração | *"Por que meu saldo na versão com threads muda a cada execução?"* | Diagnóstico da race condition (Parte 1, Fase 1) | 
| Análise de desempenho | *"Por que a soma de vetores para de acelerar a partir de 4 threads?"* | Interpretação memory-bound (Questão 6) | 

### 6.3 Análise crítica: falhas, alucinações e correção humana

O bot sugeriu falhas grosseiras sob pressão técnica que demandaram varredura e retificações brutais:

1. **Ativação inconsequente da chamada genérica `rand()`**. A IA sugeriu aplicar a função bruta sob multithreading. O sistema atolou imediatamente por serialização imposta pelos escudos protetivos implícitos da biblioteca Linux original. A reescrita demandou análise braçal e intervenção explícita na chamada blindada com apontadores focados (`rand_r`).

2. **Recomendação suicida de distribuição estática na validação de primos**. O loteamento simplista cravado pela IA derrubou drasticamente o fôlego computacional dos núcleos. Foi preciso abortar o código inicial, projetando do zero uma engenharia iterativa segmentada para estancar as perdas do escalonador.

3. **Ausência crítica de limpeza de buffer alocado no mutex**. O modelo preliminar da IA deixou blocos flutuantes nas alocações de `malloc`. Forcei varreduras duras, chamei o destruidor `pthread_mutex_destroy` e atrelei validadores manuais nas saídas, fechando frestas mortais de vazamento não referenciado de hardware.

### 6.4 Reflexão sobre aprendizagem real

Sistemas generativos entregam compilações rápidas, mas ignoram a gravidade da física computacional pesada. O assistente empacota blocos visuais perfeitos que implodem sob estresse contínuo de concorrência em kernel. O domínio da arquitetura e as premissas sistêmicas operacionais, validadas a ferro com aferições repetitivas de clock real, evidenciam que a gerência arquitetural do programador sobre a máquina segue insubstituível na extração máxima de performance sistêmica.

## 7. Conclusão e Referências

Este documento baliza categoricamente que o mero fracionamento das instruções não reflete celeridade real por osmose. Despachar sincronizações excessivas no processador gerou colapsos formidáveis na performance de escala (107x mais demorado na conta corrente isolada). O comportamento CPU-bound demonstrou força avassaladora entregando um fator positivo e massivo (6,36x de aceleração linearizada em 8 nós de Monte Carlo). Essa vitória é brutalmente estancada ao cruzar o domínio das limitações materiais da matriz nas demandas memory-bound. A trava central de arquiteturas focadas em segurança implícita (como o mecanismo GIL atuante no Python) demonstrou ser o prego final na validação ingênua de roteiros sequenciais replicados, transformando núcleos imponentes em simples gestores de fila virtual sob perda constante de poder processual real.

**Referências:**

* IEEE Std 1003.1 (POSIX), especificação de threads.

* Linux `man pthreads`, `man pthread_mutex_lock`, `man rand_r`.

* Documentação oficial do CPython - módulos `threading` e `multiprocessing`; notas sobre o GIL.

* AMDAHL, G. M. *Validity of the single processor approach to achieving large scale computing capabilities*, 1967.

* Material didático da disciplina de Computação Paralela.
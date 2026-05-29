# Algoritmo Genético para Mochila Binária - Versão MPI

## Compilação

```bash
cd mpi
make
```

Requer `mpicc` (OpenMPI ou MPICH).

## Execução

```bash
mpirun -np <num_processos> ./knapsack_mpi <entrada> <otimo> [pop] [mut] [gen]
```

## Parâmetros configuráveis

| Parâmetro | Posição | Padrão | Descrição |
|-----------|---------|--------|-----------|
| pop       | 3       | 100    | Tamanho total da população |
| mut       | 4       | 0.01   | Taxa de mutação |
| gen       | 5       | 500    | Quantidade máxima de gerações |

A quantidade de processos é definida pelo `-np` do `mpirun` e o programa suporta qualquer número de processos.

## Modelo de paralelismo

Utiliza o **modelo de ilhas**: cada processo MPI evolui sua própria subpopulação independentemente. A cada 50 gerações, os processos trocam o melhor indivíduo com o vizinho em topologia de anel (`MPI_Sendrecv`), substituindo o pior indivíduo local pelo imigrante.

## Exemplo

```bash
mpirun -np 4 ./knapsack_mpi ../entradas/large_scale/knapPI_3_100_1000_1 ../entradas/large_scale-optimum/knapPI_3_100_1000_1.txt 200 0.01 1000
```

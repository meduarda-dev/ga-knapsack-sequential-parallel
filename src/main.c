#include <stdio.h>
#include <stdlib.h>
#include "knapsack.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <arquivo_entrada> <arquivo_otimo> [pop_size] [mut_rate] [max_gen] [n_threads]\n", argv[0]);
        return 1;
    }

    GAParams params = {
        .pop_size        = (argc > 3) ? atoi(argv[3]) : 100,
        .mutation_rate   = (argc > 4) ? atof(argv[4]) : 0.01,
        .max_generations = (argc > 5) ? atoi(argv[5]) : 500,
        .n_threads       = (argc > 6) ? atoi(argv[6]) : 4
    };

    KnapsackInstance inst = load_instance(argv[1], argv[2]);

    printf("=== Problema da Mochila - Algoritmo Genetico (Paralelo) ===\n");
    printf("Itens: %d | Capacidade: %d | Otimo: %d\n", inst.n_items, inst.capacity, inst.optimum);
    printf("Populacao: %d | Mutacao: %.2f%% | Geracoes: %d | Threads: %d\n\n",
           params.pop_size, params.mutation_rate * 100, params.max_generations, params.n_threads);

    GAResult res = ga_parallel(&inst, &params);
    printf("Melhor valor: %d | Tempo: %.2f ms | Gap: %.2f%%\n",
           res.best_value, res.elapsed_ms,
           (1.0 - (double)res.best_value / inst.optimum) * 100);

    free_instance(&inst);
    return 0;
}

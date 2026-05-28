#include <stdio.h>
#include <stdlib.h>
#include "knapsack.h"

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <entrada> <otimo> [pop] [mut] [gen] [threads]\n", argv[0]);
        return 1;
    }

    GAParams params = {
        .pop_size = (argc > 3) ? atoi(argv[3]) : 100,
        .mutation_rate = (argc > 4) ? atof(argv[4]) : 0.01,
        .max_generations = (argc > 5) ? atoi(argv[5]) : 500,
        .n_threads = (argc > 6) ? atoi(argv[6]) : 4
    };

    KnapsackInstance inst = load_instance(argv[1], argv[2]);

    printf("Itens: %d | Capacidade: %d | Otimo: %d\n",
           inst.n_items, inst.capacity, inst.optimum);

    GAResult res = ga_parallel(&inst, &params);

    printf("Melhor valor: %d | Tempo: %.2f ms | Gap: %.2f%%\n",
           res.best_value,
           res.elapsed_ms,
           (1.0 - (double)res.best_value / inst.optimum) * 100);

    free_instance(&inst);
    return 0;
}
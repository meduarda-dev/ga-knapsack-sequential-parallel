#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mpi.h>

// ==================== Estruturas ====================

typedef struct {
    int value;
    int weight;
} Item;

typedef struct {
    Item *items;
    int n_items;
    int capacity;
    int optimum;
} KnapsackInstance;

typedef struct {
    int *genes;
    double fitness;
} Individual;

typedef struct {
    int pop_size;
    double mutation_rate;
    int max_generations;
} GAParams;

// ==================== Rand thread-safe ====================

static unsigned int my_rand(unsigned int *seed) {
    *seed = (*seed * 1103515245 + 12345);
    return (*seed & 0x7fffffff);
}

static double rand_double(unsigned int *seed) {
    return (double)my_rand(seed) / 2147483647.0;
}

// ==================== Carregamento ====================

KnapsackInstance load_instance(const char *input_path, const char *optimum_path) {
    KnapsackInstance inst = {0};

    FILE *f = fopen(input_path, "r");
    if (!f) { perror("Erro ao abrir entrada"); MPI_Abort(MPI_COMM_WORLD, 1); }

    fscanf(f, "%d %d", &inst.n_items, &inst.capacity);
    inst.items = malloc(inst.n_items * sizeof(Item));

    for (int i = 0; i < inst.n_items; i++)
        fscanf(f, "%d %d", &inst.items[i].value, &inst.items[i].weight);

    fclose(f);

    f = fopen(optimum_path, "r");
    if (!f) { perror("Erro ao abrir otimo"); MPI_Abort(MPI_COMM_WORLD, 1); }

    fscanf(f, "%d", &inst.optimum);
    fclose(f);

    return inst;
}

// ==================== Funções do AG ====================

static void evaluate(Individual *ind, KnapsackInstance *inst) {
    int total_value = 0, total_weight = 0;

    for (int i = 0; i < inst->n_items; i++) {
        if (ind->genes[i]) {
            total_value += inst->items[i].value;
            total_weight += inst->items[i].weight;
        }
    }

    if (total_weight <= inst->capacity) {
        ind->fitness = total_value;
    } else {
        int excesso = total_weight - inst->capacity;
        ind->fitness = total_value - excesso;
        if (ind->fitness < 0) ind->fitness = 0;
    }
}

static void init_individual(Individual *ind, KnapsackInstance *inst, unsigned int *seed) {
    int n = inst->n_items;
    ind->genes = calloc(n, sizeof(int));
    int weight = 0;

    for (int i = 0; i < n; i++) {
        int idx = my_rand(seed) % n;
        if (!ind->genes[idx] && weight + inst->items[idx].weight <= inst->capacity) {
            ind->genes[idx] = 1;
            weight += inst->items[idx].weight;
        }
    }
    evaluate(ind, inst);
}

static int tournament(Individual *pop, int pop_size, unsigned int *seed) {
    int a = my_rand(seed) % pop_size;
    int b = my_rand(seed) % pop_size;
    return (pop[a].fitness >= pop[b].fitness) ? a : b;
}

static void crossover(Individual *p1, Individual *p2, Individual *child, int n, unsigned int *seed) {
    child->genes = malloc(n * sizeof(int));
    int point = my_rand(seed) % n;
    for (int i = 0; i < n; i++)
        child->genes[i] = (i < point) ? p1->genes[i] : p2->genes[i];
}

static void mutate(Individual *ind, int n, double rate, unsigned int *seed) {
    for (int i = 0; i < n; i++) {
        if (rand_double(seed) < rate)
            ind->genes[i] ^= 1;
    }
}

static int find_best(Individual *pop, int size) {
    int best = 0;
    for (int i = 1; i < size; i++)
        if (pop[i].fitness > pop[best].fitness)
            best = i;
    return best;
}

// ==================== MPI GA - Modelo de Ilhas ====================

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (argc < 3) {
        if (rank == 0)
            printf("Uso: mpirun -np <procs> %s <entrada> <otimo> [pop] [mut] [gen]\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    GAParams params = {
        .pop_size = (argc > 3) ? atoi(argv[3]) : 100,
        .mutation_rate = (argc > 4) ? atof(argv[4]) : 0.01,
        .max_generations = (argc > 5) ? atoi(argv[5]) : 500
    };

    KnapsackInstance inst = load_instance(argv[1], argv[2]);
    int n = inst.n_items;

    // Cada processo tem sua subpopulação (ilha)
    int local_pop_size = params.pop_size / nprocs;
    if (rank < params.pop_size % nprocs)
        local_pop_size++;

    unsigned int seed = (unsigned int)time(NULL) + rank * 1000;

    // Inicializar população local
    Individual *pop = malloc(local_pop_size * sizeof(Individual));
    for (int i = 0; i < local_pop_size; i++)
        init_individual(&pop[i], &inst, &seed);

    // Intervalo de migração
    int migration_interval = 50;
    int migration_count = 1; // quantos indivíduos migram

    double t_start = MPI_Wtime();

    for (int gen = 0; gen < params.max_generations; gen++) {
        // Nova geração
        Individual *new_pop = malloc(local_pop_size * sizeof(Individual));

        // Elitismo: manter o melhor
        int best_idx = find_best(pop, local_pop_size);
        new_pop[0].genes = malloc(n * sizeof(int));
        memcpy(new_pop[0].genes, pop[best_idx].genes, n * sizeof(int));
        new_pop[0].fitness = pop[best_idx].fitness;

        // Gerar restante por crossover + mutação
        for (int i = 1; i < local_pop_size; i++) {
            int p1 = tournament(pop, local_pop_size, &seed);
            int p2 = tournament(pop, local_pop_size, &seed);
            crossover(&pop[p1], &pop[p2], &new_pop[i], n, &seed);
            mutate(&new_pop[i], n, params.mutation_rate, &seed);
            evaluate(&new_pop[i], &inst);
        }

        // Liberar população antiga
        for (int i = 0; i < local_pop_size; i++)
            free(pop[i].genes);
        free(pop);
        pop = new_pop;

        // Migração entre ilhas (anel)
        if (nprocs > 1 && gen > 0 && gen % migration_interval == 0) {
            int dest = (rank + 1) % nprocs;
            int src = (rank - 1 + nprocs) % nprocs;

            int best = find_best(pop, local_pop_size);
            int *send_genes = pop[best].genes;

            int *recv_genes = malloc(n * sizeof(int));

            MPI_Sendrecv(send_genes, n, MPI_INT, dest, 0,
                         recv_genes, n, MPI_INT, src, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            // Substituir o pior indivíduo pelo imigrante
            int worst = 0;
            for (int i = 1; i < local_pop_size; i++)
                if (pop[i].fitness < pop[worst].fitness)
                    worst = i;

            free(pop[worst].genes);
            pop[worst].genes = recv_genes;
            evaluate(&pop[worst], &inst);
        }
    }

    double t_end = MPI_Wtime();
    double elapsed_ms = (t_end - t_start) * 1000.0;

    // Encontrar melhor local
    int local_best_idx = find_best(pop, local_pop_size);
    int local_best_value = (int)pop[local_best_idx].fitness;

    // Reduzir para encontrar o melhor global
    int global_best_value;
    MPI_Reduce(&local_best_value, &global_best_value, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    double max_elapsed;
    MPI_Reduce(&elapsed_ms, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Processos MPI: %d\n", nprocs);
        printf("Itens: %d | Capacidade: %d | Otimo: %d\n", inst.n_items, inst.capacity, inst.optimum);
        printf("Pop total: %d | Mutacao: %.3f | Geracoes: %d\n",
               params.pop_size, params.mutation_rate, params.max_generations);
        printf("Melhor valor: %d | Tempo: %.2f ms | Gap: %.2f%%\n",
               global_best_value, max_elapsed,
               (1.0 - (double)global_best_value / inst.optimum) * 100);
    }

    // Cleanup
    for (int i = 0; i < local_pop_size; i++)
        free(pop[i].genes);
    free(pop);
    free(inst.items);

    MPI_Finalize();
    return 0;
}

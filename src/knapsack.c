#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "knapsack.h"

// ==================== RAND THREAD-SAFE ====================

static unsigned int rand_r(unsigned int *seed) {
    *seed = (*seed * 1103515245 + 12345);
    return (*seed & 0x7fffffff);
}

// ==================== Carregamento ====================

KnapsackInstance load_instance(const char *input_path, const char *optimum_path) {
    KnapsackInstance inst = {0};

    FILE *f = fopen(input_path, "r");
    if (!f) { perror("Erro ao abrir entrada"); exit(1); }

    fscanf(f, "%d %d", &inst.n_items, &inst.capacity);
    inst.items = malloc(inst.n_items * sizeof(Item));

    for (int i = 0; i < inst.n_items; i++)
        fscanf(f, "%d %d", &inst.items[i].value, &inst.items[i].weight);

    fclose(f);

    f = fopen(optimum_path, "r");
    if (!f) { perror("Erro ao abrir ótimo"); exit(1); }

    fscanf(f, "%d", &inst.optimum);
    fclose(f);

    return inst;
}

void free_instance(KnapsackInstance *inst) {
    free(inst->items);
}

// ==================== Funções do AG ====================

static void evaluate(Individual *ind, KnapsackInstance *inst) {
    ind->total_value = 0;
    ind->total_weight = 0;

    for (int i = 0; i < inst->n_items; i++) {
        if (ind->genes[i]) {
            ind->total_value += inst->items[i].value;
            ind->total_weight += inst->items[i].weight;
        }
    }

    // Penalização suave
    if (ind->total_weight <= inst->capacity) {
        ind->fitness = ind->total_value;
    } else {
        int excesso = ind->total_weight - inst->capacity;
        ind->fitness = ind->total_value - excesso;
        if (ind->fitness < 0) ind->fitness = 0;
    }
}

static void init_individual(Individual *ind, int n, KnapsackInstance *inst, unsigned int *seed) {
    ind->genes = calloc(n, sizeof(int));

    int weight = 0;

    // estratégia gulosa aleatória
    for (int i = 0; i < n; i++) {
        int idx = rand_r(seed) % n;

        if (!ind->genes[idx]) {
            int w = inst->items[idx].weight;

            if (weight + w <= inst->capacity) {
                ind->genes[idx] = 1;
                weight += w;
            }
        }
    }

    evaluate(ind, inst);
}

static void free_individual(Individual *ind) {
    free(ind->genes);
}

static int tournament(Individual *pop, int pop_size, unsigned int *seed) {
    int a = rand_r(seed) % pop_size;
    int b = rand_r(seed) % pop_size;
    return (pop[a].fitness >= pop[b].fitness) ? a : b;
}

static void crossover(Individual *p1, Individual *p2, Individual *child, int n, unsigned int *seed) {
    child->genes = malloc(n * sizeof(int));
    int point = rand_r(seed) % n;

    for (int i = 0; i < n; i++)
        child->genes[i] = (i < point) ? p1->genes[i] : p2->genes[i];
}

static void mutate(Individual *ind, int n, double rate, unsigned int *seed) {
    for (int i = 0; i < n; i++) {
        double r = (double)rand_r(seed) / 2147483647.0;
        if (r < rate)
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

static double time_ms(struct timespec *start, struct timespec *end) {
    return (end->tv_sec - start->tv_sec) * 1000.0 +
           (end->tv_nsec - start->tv_nsec) / 1e6;
}

// ==================== Paralelismo ====================

static void *evolve_chunk(void *arg) {
    ThreadArg *ta = (ThreadArg *)arg;
    int n = ta->instance->n_items;
    Individual *pop = ta->population;

printf("Thread trabalhando: intervalo [%d - %d)\n",
           ta->start, ta->end);

    for (int i = ta->start; i < ta->end; i++) {

        if (i == ta->start) {
    printf("[THREAD %d-%d] Gerando individuos...\n", ta->start, ta->end);
}

        int p1 = tournament(pop, ta->params->pop_size, &ta->seed);
        int p2 = tournament(pop, ta->params->pop_size, &ta->seed);

        if (i == ta->start) {
    printf("[THREAD %d-%d] Pais escolhidos: %d e %d\n",
           ta->start, ta->end, p1, p2);
}

        Individual child;
        crossover(&pop[p1], &pop[p2], &child, n, &ta->seed);
        mutate(&child, n, ta->params->mutation_rate, &ta->seed);
        evaluate(&child, ta->instance);

        if (i == ta->start) {
    printf("[THREAD %d-%d] Filho gerado com fitness: %.0f\n",
           ta->start, ta->end, child.fitness);
}

        pop[ta->params->pop_size + i] = child;
    }
    return NULL;
}

// ==================== GA ====================

GAResult ga_parallel(KnapsackInstance *inst, GAParams *params) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    int n = inst->n_items;
    int ps = params->pop_size;
    int nt = params->n_threads;
    unsigned int seed = (unsigned int)time(NULL);

    Individual *pop = calloc(ps * 2, sizeof(Individual));

    for (int i = 0; i < ps; i++)
        init_individual(&pop[i], n, inst, &seed);

    pthread_t *threads = malloc(nt * sizeof(pthread_t));
    ThreadArg *args = malloc(nt * sizeof(ThreadArg));

    for (int gen = 0; gen < params->max_generations; gen++) {

        int best_idx = find_best(pop, ps);

        if (gen % 50 == 0) {
    printf("\n=== GERACAO %d ===\n", gen);
    printf("Melhor atual: %.0f\n", pop[best_idx].fitness);
}
        pop[ps].genes = malloc(n * sizeof(int));
        memcpy(pop[ps].genes, pop[best_idx].genes, n * sizeof(int));
        evaluate(&pop[ps], inst);

        int work = ps - 1;
        int chunk = work / nt;
        int remainder = work % nt;
        int offset = 1;

        for (int t = 0; t < nt; t++) {
            int size = chunk + (t < remainder ? 1 : 0);

            args[t] = (ThreadArg){
                .population = pop,
                .instance = inst,
                .params = params,
                .start = offset,
                .end = offset + size,
                .seed = seed + t + gen
            };

            pthread_create(&threads[t], NULL, evolve_chunk, &args[t]);
            offset += size;
        }

        for (int t = 0; t < nt; t++)
            pthread_join(threads[t], NULL);

        for (int i = 0; i < ps; i++)
            free_individual(&pop[i]);

        for (int i = 0; i < ps; i++) {
            pop[i] = pop[ps + i];
            pop[ps + i].genes = NULL;
        }
    }

    int best = find_best(pop, ps);

    GAResult result = {
        .best_value = (int)pop[best].fitness
    };

    for (int i = 0; i < ps; i++)
        free_individual(&pop[i]);

    free(pop);
    free(threads);
    free(args);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    result.elapsed_ms = time_ms(&t0, &t1);

    return result;
}
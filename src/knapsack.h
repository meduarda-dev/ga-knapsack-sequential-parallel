#ifndef KNAPSACK_H
#define KNAPSACK_H

#include <pthread.h>

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
    int total_value;
    int total_weight;
    double fitness;
} Individual;

typedef struct {
    int pop_size;
    double mutation_rate;
    int max_generations;
    int n_threads;
} GAParams;

typedef struct {
    Individual *population;
    KnapsackInstance *instance;
    GAParams *params;
    int start;
    int end;
    unsigned int seed;
} ThreadArg;

typedef struct {
    int best_value;
    double elapsed_ms;
} GAResult;

KnapsackInstance load_instance(const char *input_path, const char *optimum_path);
void free_instance(KnapsackInstance *inst);
GAResult ga_parallel(KnapsackInstance *inst, GAParams *params);

#endif
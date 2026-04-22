CC = gcc
CFLAGS = -Wall -O2 -pthread
SRC = src/main.c src/knapsack.c
OUT = knapsack_ga

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

# Exemplo de execução com a entrada de 100 itens:
# ./knapsack_ga entradas/large_scale/knapPI_3_100_1000_1 entradas/large_scale-optimum/knapPI_3_100_1000_1.txt 200 0.01 1000 4

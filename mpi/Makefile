CC = mpicc
CFLAGS = -Wall -O2
SRC = knapsack_mpi.c
OUT = knapsack_mpi

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

# Exemplos de execução:
# mpirun -np 4 ./knapsack_mpi ../entradas/large_scale/knapPI_3_100_1000_1 ../entradas/large_scale-optimum/knapPI_3_100_1000_1.txt 200 0.01 1000
# mpirun -np 8 ./knapsack_mpi ../entradas/large_scale/knapPI_3_1000_1000_1 ../entradas/large_scale-optimum/knapPI_3_1000_1000_1.txt 400 0.02 500

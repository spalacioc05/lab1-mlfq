CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = main.c queue.c scheduler.c csv.c
TEST_SRC = tests.c queue.c scheduler.c csv.c
# experiments y tests no exportan CSV: solo necesitan el dominio.
EXP_SRC = experiments.c queue.c scheduler.c

ifeq ($(OS),Windows_NT)
    TARGET = mlfq.exe
    TEST_TARGET = tests.exe
    EXP_TARGET = experiments.exe
    RM = del /Q
else
    TARGET = mlfq
    TEST_TARGET = tests
    EXP_TARGET = experiments
    RM = rm -f
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(TEST_TARGET): $(TEST_SRC)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRC)

$(EXP_TARGET): $(EXP_SRC)
	$(CC) $(CFLAGS) -o $(EXP_TARGET) $(EXP_SRC)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

# El target es run-experiments (y no experiments) para no chocar con el
# nombre del ejecutable en Linux.
run-experiments: $(EXP_TARGET)
	./$(EXP_TARGET)

clean:
	$(RM) $(TARGET) $(TEST_TARGET) $(EXP_TARGET) results.csv

.PHONY: all test run-experiments clean

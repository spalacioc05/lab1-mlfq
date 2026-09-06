CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRC = main.c queue.c scheduler.c csv.c
TEST_SRC = tests.c queue.c scheduler.c csv.c

ifeq ($(OS),Windows_NT)
    TARGET = mlfq.exe
    TEST_TARGET = tests.exe
    RM = del /Q
else
    TARGET = mlfq
    TEST_TARGET = tests
    RM = rm -f
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

$(TEST_TARGET): $(TEST_SRC)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRC)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	$(RM) $(TARGET) $(TEST_TARGET) results.csv

.PHONY: all test clean

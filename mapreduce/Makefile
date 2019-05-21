CC = mpicxx
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)
BIN = ./bin
TARGET = mapreduce.out

LDFLAGS = -lm
CFLAGS = -I./include -g -Wall -O3

all: dir $(BIN)/$(TARGET)

run:
	mpirun $(BIN)/$(TARGET)

dir: ${BIN}

${BIN}:
	mkdir -p $(BIN)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN)/$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS) 

.PHONY: clean
clean:
	rm -f $(OBJ) $(BIN)/$(TARGET)

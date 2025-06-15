
SRC_DIR=src
SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ_DIR=obj
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))
BIN_DIR=bin
BIN=$(BIN_DIR)/disekt

CC=gcc
CPPFLAGS=-Iinclude
CFLAGS=-g -Wall
#LDLIBS=-lSDL2
LDLIBS=-lm -lraylib

.PHONY: run clean 

run: $(BIN)
	$(BIN) disks/memtest_1999.d64

$(BIN): $(OBJ) | $(BIN_DIR)
	$(CC) $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean:
	$(RM) -rv $(BIN_DIR) $(OBJ_DIR) 

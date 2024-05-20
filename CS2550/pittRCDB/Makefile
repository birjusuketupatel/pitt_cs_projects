# Variables
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := ./
OUT	:= pittRCDB
EXE := $(BIN_DIR)/$(OUT)
SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

LDLIBS := -lstdc++fs

# Compiler Settings
CC := g++
CFLAGS := -g -c -Wall --std=c++17

.PHONY: all clean

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC)  $^ $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC)  $(CFLAGS) -c $< -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

run: $(EXE)
	$(EXE)

-include $(OBJ:.o=.d)

clean:
	@$(RM) -rv $(OBJ_DIR) tables
	@$(RM) -rv logs/*
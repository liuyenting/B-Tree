# ====================
# Envirnoment setup
# ====================
CXX = g++-4.9
CFLAGS = -Wall -O3 -std=c++11

# Directories
SRC_DIR := src/
OBJ_DIR := obj/
BIN_DIR := bin/

# Source files in sequence
SRC_FILES := src/demo.cpp
OBJ_FILES := $(addprefix $(OBJ_DIR),$(notdir $(SRC_FILES:.cpp=.o)))

# Main executable
MAIN = dsa_hw2-4
# ====================


# ====================
# Help Configurations
# ====================
.DEFAULT_GOAL := help
help:
	@echo
	@echo "===== Makefile Help Message ====="
	@echo
	@echo "all\t\tAn alias for 'build_local'."
	@echo "debug\t\tBuild the project with debug flag being set."
	@echo
	@echo "build_local\tBuild the project from '$(SRC_DIR)', using LOCAL path."
	@echo "build_remote\tSend the entire project to workstation and build there."
	@echo
	@echo "upload\t\tUpload the compiled binary to server."
	@echo
	@echo "run\t\tAn alias for 'run_local'."
	@echo "run_local\tRun the binary locally."
	@echo "run_remote\tRun the binary on the workstation."
	@echo
	@echo "clean\t\tWipe out all the object files and binaries."
	@echo
	@echo "help\t\tShow this help message."
	@echo
	@echo "* Default action for 'make' is set to '$(.DEFAULT_GOAL)'."
	@echo
	@echo "================================="
	@echo
# ====================


# ====================
# Compile related
# ====================
all: clean build_local
	@echo "Complete!"

build_local: CFLAGS += -DLOCAL
build_local: clean $(MAIN)
	@echo "Binary based on LOCAL path is built."

build_remote: CFLAGS += -DREMOTE
build_remote: 
	# send make file to server
	# execute make file
	@echo "Binary based on REMOTE path is built."

$(MAIN): $(OBJ_FILES)
	@echo "Linking all the object files..."
	@$(CXX) $(CFLAGS) $(INCLUDES) -o $(BIN_DIR)$@ $^ $(LFLAGS)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@echo "Compiling $<..." 
	@$(CXX) $(CFLAGS) $(INCLUDES) -c -o $@ $<

debug: CFLAGS += -DDEBUG
debug: all
	@mv $(BIN_DIR)$(MAIN) $(BIN_DIR)$(MAIN)_debug
# ====================


# ====================
# Workstation related
# ====================
upload:

run_remote:
# ====================


# ====================
# Local related
# ====================
run: run_local

run_local:
ifneq ($(wildcard $(BIN_DIR)$(MAIN)),)
	./$(BIN_DIR)$(MAIN)
else ifneq ($(wildcard $(BIN_DIR)$(MAIN)_debug),)
	./$(BIN_DIR)$(MAIN)_debug
else
	@echo "Please execute 'make all' or 'make debug' first."
endif

clean:
	@rm -rf obj/*
	@echo "'obj' wiped..."
	@rm -rf bin/*
	@echo "'bin' wiped..."
# ====================

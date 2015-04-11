# ====================
# Envirnoment setup
# ====================
CXX = g++-4.9

# Directories
SRC_DIR := src/
OBJ_DIR := obj/
BIN_DIR := bin/
DAT_DIR := dat/

# Setup CXXFLAGS for LOCAL/REMOTE differences
ifeq ($(wildcard ./$(DAT_DIR)),)
	CXXFLAGS = -DREMOTE
else
	CXXFLAGS = -DLOCAL
endif
override CXXFLAGS += -Wall -O3 -std=c++11 -fopenmp -DMMF

# Create directories if not exist
$(OBJ_DIR):
	@echo "Create OBJ_DIR at '$(OBJ_DIR)'."
	@mkdir $@

$(BIN_DIR):
	@echo "Create BIN_DIR at '$(BIN_DIR)'."
	@mkdir $@

# Source files in sequence
SRC_FILES := src/demo.cpp
OBJ_FILES := $(addprefix $(OBJ_DIR),$(notdir $(SRC_FILES:.cpp=.o)))

# Main executable
MAIN = dsa_hw2-4

# Workstation setup
KEY_FILE = key/csie_workstation
ACCOUNT = b03902036
SERVER = linux1.csie.ntu.edu.tw

# Remote command
REMOTE_BASE_DIR = ~/DSA
COMMAND =
# ====================


# ====================
# Help Configurations
# ====================
.DEFAULT_GOAL := help
help:
	@echo
	@echo "===== Makefile Help Message ====="
	@echo
	@echo "all\t\tBuild and run the project."
	@echo
	@echo "build\t\tBuild the project from '$(SRC_DIR)'."
	@echo "debug\t\tBuild the project with debug flag being set."
	@echo "run\t\tRun the binary locally."
	@echo "clean\t\tWipe out all the object files and binaries."
	@echo
	@echo "upload\t\tUpload the project to workstation."
	@echo
	@echo "help\t\tShow this help message."
	@echo
	@echo "* Default action for 'make' is set to '$(.DEFAULT_GOAL)'."
	@echo "* Use 'remote_' prefix for workstation access, e.g. remote_run"
	@echo
	@echo "================================="
	@echo
# ====================


# ====================
# Compile related
# ====================
all: build run

build: $(BIN_DIR) $(OBJ_DIR) $(MAIN)
	@echo "Compile complete."

debug: CXXFLAGS += -DDEBUG
debug: build
	@mv $(BIN_DIR)$(MAIN) $(BIN_DIR)$(MAIN)_debug

$(MAIN): $(OBJ_FILES)
	@echo "Linking all the object files..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(BIN_DIR)$@ $^ $(LFLAGS)

$(OBJ_DIR)%.o: $(SRC_DIR)%.cpp
	@echo "Compiling $<..." 
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<
# ====================


# ====================
# Workstation related
# ====================
upload:
	@echo "Uploading to $(ACCOUNT)@$(SERVER)..."
	@rsync -e 'ssh -i $(KEY_FILE)' --exclude-from '.gitignore' -avP * $(ACCOUNT)@$(SERVER):~/DSA/

remote_build: COMMAND = make build
remote_build: remote

remote_debug: COMMAND = make debug
remote_debug: remote

remote_run: COMMAND = make run
remote_run: remote

remote_clean: COMMAND = make clean
remote_clean: remote

remote:
	@ssh -i $(KEY_FILE) $(ACCOUNT)@$(SERVER) "cd $(REMOTE_BASE_DIR) && $(COMMAND)"
# ====================


# ====================
# Local related
# ====================
run:
ifneq ($(wildcard $(BIN_DIR)$(MAIN)),)
	@./$(BIN_DIR)$(MAIN)
else ifneq ($(wildcard $(BIN_DIR)$(MAIN)_debug),)
	@./$(BIN_DIR)$(MAIN)_debug
else
	@echo "Please execute 'make all' or 'make debug' first."
endif

clean:
	@rm -rf $(OBJ_DIR)
	@echo "'$(OBJ_DIR)' wiped..."
	@rm -rf $(BIN_DIR)
	@echo "'$(BIN_DIR)' wiped..."
# ====================

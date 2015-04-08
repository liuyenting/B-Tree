# Showing the help message if nothing set
.DEFAULT_GOAL := help
help:
	@echo
	@echo "===== Makefile Help Message ====="
	@echo
	@echo "all\t\tBuild the project using source code under 'src' folder."
	@echo "debug\t\tBuild the project with debug flag being set."
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
	@echo "================================="
	@echo

#
# Compile related
#
all:

debug:


#
# Workstation related
#
upload:

run_remote:


#
# Local related
#
run:
	@run_local

run_local:

clean:
	@echo
	@rm -rf obj/*
	@echo "'obj' wiped..."
	@rm -rf bin/*
	@echo "'bin' wiped..."
	@echo

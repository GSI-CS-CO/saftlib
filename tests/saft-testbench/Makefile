all: print_system
	@echo "Running test cases..."
	./saft-testbench_test.sh
	./saft-ctl_test.sh

print_system:
	@echo "Current system:"
	uname -a
	lscpu
	@echo ""

clean:
	rm *.txt || true


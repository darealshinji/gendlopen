BIN := src/gendlopen


.PHONY: all clean clean-test test run_tests run_tests2

all: $(BIN)

clean: clean-test
	$(MAKE) -C src clean

clean-test:
	$(MAKE) -C examples clean

$(BIN):
	$(MAKE) -C src

test: $(BIN)
	$(MAKE) -C examples

run_tests: $(BIN)
	$(MAKE) -C examples $@

run_tests2: $(BIN)
	$(MAKE) -C examples $@


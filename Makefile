INC=-Iinclude
CFLAGS=-Wall -g -O0 $(INC)

.PHONY: test
test: run_tests

parbake:
	$(MAKE) -C src

run_tests: test.o parbake
	$(CC) -o $@ $< src/parbake.a

test.o: test.c

.PHONY : clean
clean :
	@rm run_tests test.o 2>/dev/null || true
	$(MAKE) -C src clean

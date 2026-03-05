# Run MFCC then cross-validate with ground_truth.h
CC = gcc
CFLAGS = -O2 -Wall
LDFLAGS = -lm
VALIDATOR = run_test

.PHONY: all run clean

all: $(VALIDATOR)

$(VALIDATOR): run_test.c
	$(CC) $(CFLAGS) -o $(VALIDATOR) run_test.c $(LDFLAGS)

run: $(VALIDATOR)
	$(MAKE) -C ../mfcc
	../mfcc/mfcc
	./$(VALIDATOR)

clean:
	rm -f $(VALIDATOR) mfcc_output.txt

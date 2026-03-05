# Standalone x86: GAP 16-bit fixed-point MFCC. All sources under this folder (no SDK paths).
CC     = gcc
CFLAGS = -O2 -D__EMUL__ -std=gnu99 -Wall
INCS   = -I. -Isdk -Iluts -I../validated_input_ground_truth

SDK_C  = sdk/PreProcessing.c sdk/FFT_Library.c sdk/CmplxFunctions.c \
         sdk/MfccBasicKernels.c sdk/math_funcs.c
OBJS   = main.o PreProcessing.o FFT_Library.o CmplxFunctions.o MfccBasicKernels.o math_funcs.o

all: mfcc

mfcc: $(OBJS)
	$(CC) -o $@ $(OBJS) -lm

main.o: main.c luts.h
	$(CC) $(CFLAGS) $(INCS) -c -o $@ main.c

PreProcessing.o: sdk/PreProcessing.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

FFT_Library.o: sdk/FFT_Library.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

CmplxFunctions.o: sdk/CmplxFunctions.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

MfccBasicKernels.o: sdk/MfccBasicKernels.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

math_funcs.o: sdk/math_funcs.c
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

clean:
	rm -f mfcc $(OBJS) mfcc_output.txt

.PHONY: all clean

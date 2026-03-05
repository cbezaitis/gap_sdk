/*
 * Standalone 16-bit fixed-point MFCC on x86.
 * Core MFCC pipeline only (no testing/IO here).
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define MFCC_NORM       6
#define N_FRAME         49
/* Number of int16 coefficients per frame produced by Librosa_MFCC */
#define MFCC_FRAME_FEATS 80

/* Single definition for L2/L3 transfer counters (declared extern in include/) */
unsigned int __L3_Read, __L3_Write, __L2_Read, __L2_Write;

#include "MFCC_params.h"
#include "MfccKernels.h"
#include "WindowLUT.def"
#include "RFFTTwiddles.def"
#include "FFTTwiddles.def"
#include "SwapTable.def"
#include "DCTTwiddles.def"
#include "MelFBCoeff.def"
#include "MelFBSparsity.def"
#include "wav_data.h"

typedef int16_t INOUT_TYPE;

static INOUT_TYPE *out_feat;
static INOUT_TYPE *MfccInSig;

/* Forward declaration for optional testing helpers in testing.c */
void mfcc_after_run(const INOUT_TYPE *out, int n_frames, int frame_size);

static int clip_int16(int x, int prec)
{
	int min = -(1 << prec);
	int max = (1 << prec) - 1;
	if (x < min) return min;
	if (x > max) return max;
	return x;
}

static void RunMFCC(void)
{
	int frame_size = MFCC_FRAME_FEATS;

	printf("run mfcc (16-bit fixed-point)\n");
	Librosa_MFCC(MfccInSig, out_feat,
	             FFTTwiddles, SwapTable, WindowLUT,
	             MelFBSparsity, MelFBCoeff, MFCC_NORM, DCTTwiddles);

	mfcc_after_run(out_feat, N_FRAME, frame_size);
}

int main(void)
{
	int frame_size = MFCC_FRAME_FEATS;
	int num_samples = wav_samples_len;

	printf("\n *** MFCC Standalone (16-bit fix, x86, embedded wav_data) ***\n\n");

	/* Allocate L1/L2 used by generated kernel */
	L1_Memory = (char *)malloc(_L1_Memory_SIZE);
	L2_Memory = (char *)malloc(_L2_Memory_SIZE > 0 ? _L2_Memory_SIZE : 1);
	if (!L1_Memory || !L2_Memory) {
		printf("Error allocating L1/L2\n");
		return -1;
	}

	out_feat  = (INOUT_TYPE *)malloc(N_FRAME * frame_size * sizeof(INOUT_TYPE));
	MfccInSig = (INOUT_TYPE *)malloc(num_samples * sizeof(INOUT_TYPE));
	if (!out_feat || !MfccInSig) {
		printf("Error allocating buffers\n");
		return -1;
	}

	/* Convert static wav_samples[] into Q15 input buffer */
	for (int i = 0; i < num_samples; i++) {
		MfccInSig[i] = (INOUT_TYPE)clip_int16((int)wav_samples[i], 15);
	}

	RunMFCC();

	free(L1_Memory);
	free(L2_Memory);
	free(out_feat);
	free(MfccInSig);

	return 0;
}


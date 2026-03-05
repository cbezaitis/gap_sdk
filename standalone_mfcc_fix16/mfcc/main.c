/*
 * Standalone x86: GAP 16-bit fixed-point MFCC (exact SDK behavior).
 * All sources are under this folder (sdk/, luts/); no external SDK paths.
 * Pipeline: PreEmphasis -> WindowingReal2Real_PadCenter_Fix16 -> RFFT_DIF_Seq_Fix16
 *           -> CmplxMag_Fix32 -> MelFilterBank_Fix32 -> MFCC_ComputeDB_Fix32 -> MFCC_ComputeDCT_II_Fix16.
 * Build: make
 */
#define __EMUL__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "DSP_Lib.h"
#include "../validated_input_ground_truth/input_wav.h"

#define FRAME_SIZE   400
#define FRAME_STEP   160
#define N_FFT        512
#define N_MELS       40
#define NORM         6
#define Q_IN_FFT     12
#define MEL_COEFF_DYN 15

/* LUTs from gap_sdk/examples/gap8/dsp/Mfcc/BUILD_MODEL (PI_L2 stripped) */
#include "luts.h"

static int num_frames(int n) {
    if (n < FRAME_SIZE) return 0;
    return (n - FRAME_SIZE) / FRAME_STEP + 1;
}

int main(void) {
    int n_frames = num_frames(wav_samples_len);
    if (n_frames <= 0) {
        fprintf(stderr, "Not enough samples: %d\n", wav_samples_len);
        return 1;
    }

    /* Buffers per frame (GAP pipeline layout) */
    short int    frame_in[FRAME_SIZE];
    short int    preemp_out[FRAME_SIZE];
    short int    windowed[N_FFT];
    short int    rfft_out[514];   /* 257 complex */
    unsigned int mag[257];
    unsigned int mel_spectr[N_MELS];
    signed char  shift_buff[N_MELS];
    short int    log_mel[N_MELS];
    short int    mfcc_out[N_MELS];

    short int    preemp_shift;
    short int   prev = 0;

    PreEmphasis_T         preemp_arg;
    Windowing_T           win_arg;
    MelFilterBank_T       mel_arg;
    MFCC_Log_T            log_arg;
    DCT_II_Arg_T          dct_arg;
    CmplxMag_T            cmag_arg;

    FILE *fp = fopen("mfcc_output.txt", "w");
    if (!fp) {
        fprintf(stderr, "Cannot create mfcc_output.txt\n");
        return 1;
    }

    for (int f = 0; f < n_frames; f++) {
        int offset = f * FRAME_STEP;
        for (int i = 0; i < FRAME_SIZE; i++)
            frame_in[i] = (short int) wav_samples[offset + i];

        /* 1. PreEmphasis (includes get_max, sets preemp_shift) */
        memset(preemp_arg.maxin, 0, sizeof(preemp_arg.maxin));
        preemp_arg.Frame     = frame_in;
        preemp_arg.Out       = preemp_out;
        preemp_arg.Prev      = prev;
        preemp_arg.PreempFactor = 0;
        preemp_arg.Shift     = &preemp_shift;
        preemp_arg.QIn_FFT   = Q_IN_FFT;
        preemp_arg.FrameSize = FRAME_SIZE;
        PreEmphasis(&preemp_arg);
        prev = frame_in[FRAME_SIZE - 1];

        /* 2. WindowingReal2Real_PadCenter_Fix16: 400 -> 512 (center pad + window Q15) */
        win_arg.Frame     = preemp_out;
        win_arg.OutFrame  = windowed;
        win_arg.Window    = (void *) WindowLUT;
        win_arg.FrameSize = FRAME_SIZE;
        win_arg.FFT_Dim   = N_FFT;
        WindowingReal2Real_PadCenter_Fix16(&win_arg);

        /* 3. RFFT_DIF_Seq_Fix16 (overwrites windowed; output in rfft_out) */
        RFFT_DIF_Seq_Fix16(windowed, rfft_out, (short int *)FFTTwiddles, (short int *)RFFTTwiddles, (short int *)SwapTable, N_FFT);

        /* 4. CmplxMag_Fix32: magnitude (not power); ExtraQ from preemp shift */
        cmag_arg.FrameIn       = rfft_out;
        cmag_arg.FrameOut      = (void *) mag;
        cmag_arg.Nfft          = N_FFT;
        cmag_arg.ExtraQ        = (short int) preemp_shift;
        cmag_arg.Input_QFormat = 8;
        cmag_arg.shift_fft     = NULL;
        CmplxMag_Fix32(&cmag_arg);

        /* 5. MelFilterBank_Fix32 (writes shift_buff) */
        mel_arg.FramePower    = (void *) mag;
        mel_arg.MelSpectr     = (void *) mel_spectr;
        mel_arg.Mel_Coeffs    = (void *) MelFBCoeff;
        mel_arg.shift_buff    = shift_buff;
        mel_arg.shift_fft     = NULL;
        mel_arg.Mel_FilterBank = (short int *) MelFBSparsity;  /* same layout as fbank_type_t[40] */
        mel_arg.Mel_NBanks    = N_MELS;
        mel_arg.Mel_Coeff_dyn = MEL_COEFF_DYN;
        mel_arg.IsMagSquared  = 0;
        MelFilterBank_Fix32(&mel_arg);

        /* 6. MFCC_ComputeDB_Fix32: 10*log10 style, IsMagSquared=0 */
        log_arg.FrameIn      = (void *) mel_spectr;
        log_arg.FrameOut     = (void *) log_mel;
        log_arg.FrameSize    = N_MELS;
        log_arg.Norm         = NORM;
        log_arg.ExtraQ       = (short int) preemp_shift;
        log_arg.Q_FFT_Out    = 8;
        log_arg.Mel_Coeff_Dyn = MEL_COEFF_DYN;
        log_arg.IsMagSquared = 0;
        log_arg.shift_buff   = shift_buff;
        log_arg.LogOffset    = 0;
        MFCC_ComputeDB_Fix32(&log_arg);

        /* 7. MFCC_ComputeDCT_II_Fix16 -> Q2 output (Norm 14, clip 15 in kernel) */
        dct_arg.Data     = (void *) log_mel;
        dct_arg.DCTCoeff = (void *) DCTTwiddles;
        dct_arg.FeatList = (void *) mfcc_out;
        dct_arg.n_input  = N_MELS;
        dct_arg.n_dct     = N_MELS;
        MFCC_ComputeDCT_II_Fix16(&dct_arg);

        for (int j = 0; j < N_MELS; j++) {
            fprintf(fp, "%d", (int) mfcc_out[j]);
            if (j < N_MELS - 1) fprintf(fp, " ");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    return 0;
}

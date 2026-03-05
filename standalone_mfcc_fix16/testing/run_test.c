/*
 * Cross-validate MFCC fix16 output against ground_truth (float).
 * Fix16 is Q2: float_value = fix16_value / 4.0 (MfccRunTest QMFCC=15-NORM-7=2).
 * QSNR = 10*log10(SUM(ground^2) / MSE), threshold 38 for fix16.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../validated_input_ground_truth/ground_truth.h"

#define N_DCT           40
#define QSNR_THRESHOLD  38.0f
#define FIX2FP_SCALE    4.0   /* Q2 */

int main(void) {
    size_t gt_count = sizeof(ground_truth) / sizeof(ground_truth[0]);
    int n_frames = (int)(gt_count / N_DCT);
    if (n_frames <= 0) {
        fprintf(stderr, "ground_truth size invalid: %zu\n", gt_count);
        return 1;
    }

    FILE *fp = fopen("mfcc_output.txt", "r");
    if (!fp) {
        fprintf(stderr, "Cannot open mfcc_output.txt (run ../mfcc/mfcc from this dir first)\n");
        return 1;
    }

    double MSE = 0.0, SUM = 0.0;
    int idx = 0;
    char line[4096];
    while (fgets(line, sizeof(line), fp) && idx < n_frames * N_DCT) {
        int val[N_DCT];
        int n, k;
        char *p = line;
        for (n = 0; n < N_DCT; n++) {
            long v = strtol(p, &p, 10);
            val[n] = (int)v;
        }
        for (k = 0; k < N_DCT && idx < (int)gt_count; k++, idx++) {
            double f = (double)val[k] / FIX2FP_SCALE;
            double diff = ground_truth[idx] - f;
            MSE += diff * diff;
            SUM += ground_truth[idx] * ground_truth[idx];
        }
    }
    fclose(fp);

    if (idx != (int)gt_count) {
        fprintf(stderr, "Frame count mismatch: got %d coeffs, expected %zu\n", idx, gt_count);
        return 1;
    }

    double QSNR = (MSE > 0 && SUM > 0) ? (10.0 * log10(SUM / MSE)) : 0.0;
    printf("QSNR: %f (thr: %f) --> ", QSNR, (double)QSNR_THRESHOLD);
    if (QSNR < QSNR_THRESHOLD) {
        printf("Test NOT PASSED\n");
        return -1;
    }
    printf("Test PASSED\n");
    return 0;
}

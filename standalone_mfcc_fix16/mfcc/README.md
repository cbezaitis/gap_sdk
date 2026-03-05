# Standalone 16-bit MFCC (x86)

No GAP8 or SDK runtime is needed.
Runs the 16-bit fixed-point MFCC pipeline on x86 using **make**, **gcc**, and **libm**. 

Input is the embedded waveform in `gap_sdk/standalone_mfcc_fix16/validated_input_ground_truth/input_wav.h`.
The mfcc output is the txt file. 
The txt file is deleted from make clean, so the experiment can be replicated. 

```bash
make clean
make run 
```

# Testing of standalone 

The mfcc_output is in the folder "gap_sdk/standalone_mfcc_fix16/mfcc"
The current executable cross-checks from the "gap_sdk/standalone_mfcc_fix16/validated_input_ground_truth/ground_truth.h" 
and calculates the QSNR with the relavant threshold. 

```bash
make clean
make run 
```

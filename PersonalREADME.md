## GAP SDK Docker image (`gap`)

This section documents how the `gap` Docker image was created from this repository and how to reproduce the environment and test run.

### What the image contains

- **Base image**: `ubuntu:20.04` (matches the SDK’s documented environment).
- **System packages**: all packages listed under “OS Requirements installation” in `README.md` / `doc/source/getting_started.rst`, including `autoconf`, `automake`, `build-essential`, `cmake`, `git`, `python3-pip`, `scons`, `texinfo`, `wget`, USB/dev libraries, SDL2, GraphicsMagick, etc.
- **Python setup**:
  - `python` is configured to point to `python3` via `update-alternatives`.
  - Python dependencies from `requirements.txt` and `doc/requirements.txt` are installed with `pip3`.
  - Extra DSP-related Python packages are preinstalled: **`numpy`** and **`librosa`**, so MFCC LUT generation and ground-truth scripts work out of the box.
- **GAP RISC‑V toolchain**:
  - The official repo `https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu.git` is cloned in the image build.
  - Instead of running its `install.sh` with `sudo` (not available in Docker build), the image:
    - Creates `/usr/lib/gap_riscv_toolchain`.
    - Uses `rsync -av --delete --exclude ".git*"` to copy the toolchain contents there, replicating what `install.sh` does as root.
- **SDK source**:
  - The contents of this repository are copied into `/opt/gap_sdk`.
  - The working directory in the image is set to `/opt/gap_sdk`.

### Dockerfile (summary)

The `Dockerfile` at the root of this repo does the following:

1. `FROM ubuntu:20.04` and sets `DEBIAN_FRONTEND=noninteractive`.
2. `apt-get update` and installs all SDK prerequisites.
3. Runs `update-alternatives --install /usr/bin/python python /usr/bin/python3 10`.
4. Clones the GAP RISC‑V toolchain repo and `rsync`s it into `/usr/lib/gap_riscv_toolchain`.
5. `WORKDIR /opt/gap_sdk` and `COPY . .` to bring in this SDK.
6. `pip3 install --upgrade pip` and installs requirements from:
   - `requirements.txt`
   - `doc/requirements.txt`
7. Sets the default container command to `/bin/bash` for interactive use.

### How to build the image

From the root of the SDK repo (`gap_sdk` directory), run:

```bash
docker build -t gap .
```

This produces a local Docker image named `gap`.

### How to run a basic test inside the image

The following commands run the PMSIS HelloWorld example on the GVSOC simulator entirely inside the `gap` image and verify that the SDK is working:

```bash
docker run --rm gap bash -lc \
  "cd /opt/gap_sdk && \
   source sourceme.sh --board gapuino && \
   make -f gap8/gap8.mk gvsoc && \
   cd examples/gap8/basic/helloworld && \
   make clean all run PMSIS_OS=freertos platform=gvsoc io=host"
```

What this does:

- **`source sourceme.sh --board gapuino`**:
  - Sets up `GAP_SDK_HOME` and other environment variables.
  - Selects the GAP8 GAPUINO_V3 board configuration (interactive menu is driven by `sourceme.sh`).
- **`make -f gap8/gap8.mk gvsoc`**:
  - Builds the GVSOC simulation platform and its models for GAP8 inside the container.
- **`cd examples/gap8/basic/helloworld`** and `make clean all run PMSIS_OS=freertos platform=gvsoc io=host`:
  - Builds the FreeRTOS-based HelloWorld example for GAP8 using the installed toolchain and SDK.
  - Runs it on GVSOC with host I/O so that output appears in the container’s console.

Expected output snippet (at the end of the run):

```text
 *** PMSIS HelloWorld ***

Entering main controller
[32 0] Hello World!
...
Cluster master core exit
Test success !
```

Seeing `Test success !` confirms that:

- The Docker image `gap` was built correctly.
- The GAP SDK, toolchain, and GVSOC are functioning inside the container.

### MFCC examples inside the Docker image

The MFCC examples live under `examples/gap8/dsp/Mfcc` and rely on the Autotiler v3 DSP generators. The rebuilt `gap` image already contains the Python packages (`numpy`, `librosa`) required by `DSP_LUTGen.py` and the ground‑truth script.

All commands below assume you are in the SDK root inside the container (`/opt/gap_sdk`) and have sourced the environment.

#### 1. Enter the image and source the SDK

```bash
docker run --rm -it gap bash

cd /opt/gap_sdk
source sourceme.sh --board gapuino
```

This selects the GAP8 GAPUINO_V3 board and prepares `GVSOC` and the toolchain.

#### 2. Build GVSOC once (if not already built)

```bash
make -f gap8/gap8.mk gvsoc
```

You only need to do this once per SDK tree inside the container; subsequent MFCC runs can reuse the same GVSOC build.

#### 3. Run the 16‑bit fixed‑point MFCC example (GVSOC)

From the SDK root inside the container:

```bash
cd /opt/gap_sdk/examples/gap8/dsp/Mfcc
make clean all run PMSIS_OS=freertos platform=gvsoc io=host
```

- **Expected behavior**:
  - Generates LUTs and MFCC kernels using `MfccConfig.json` (`dtype: fix16`).
  - Builds and runs on GVSOC.
  - Prints a summary similar to:

    ```text
     *** MFCC Test ***

    ...
    FIXED POINT
    run mfcc
    Total Cycles: 489219 over 49 Frames 9984 Cyc/Frame
    QSNR: 39.642662 (thr: 38.000000) --> Test PASSED
    ```

#### 4. Run the 32‑bit fixed‑point MFCC example (GVSOC)

To run the 32‑bit fixed‑point variant generated from `MfccConfig_fix32.json` (`dtype: fix32_scal`), use:

```bash
cd /opt/gap_sdk/examples/gap8/dsp/Mfcc
make clean all run \
  PMSIS_OS=freertos platform=gvsoc io=host \
  MFCC_PARAMS_JSON=MfccConfig_fix32.json \
  EXTRA_APP_CFLAGS='-DHIGH_PREC_FFT'
```

- **Notes**:
  - This regenerates LUTs and kernels for the 32‑bit fixed‑point configuration and runs on GVSOC.
  - With the current configuration and reference ground truth, the **32‑bit fixed‑point test executes but does not reach the QSNR threshold** (for reference, the host emulation run reports `QSNR ≈ 3.01` vs threshold `38.0`, i.e. `Test NOT PASSED`). The 16‑bit configuration passes.

#### 5. Optional: host emulation of MFCC (no GVSOC)

You can also run the MFCC pipeline on the host using the emulation makefile:

```bash
cd /opt/gap_sdk/examples/gap8/dsp/Mfcc

# 16‑bit fixed‑point emulation
make -f Emul.mk clean all
./mfcc_emul samples/yes.wav

# 32‑bit fixed‑point emulation
make -f Emul.mk clean all MFCC_PARAMS_JSON=MfccConfig_fix32.json EXTRA_APP_CFLAGS='-DHIGH_PREC_FFT'
./mfcc_emul samples/yes.wav
```

The emulation binary prints the same MFCC test banner and QSNR information without requiring GVSOC.

### Prebuilt Docker image (for users who cannot rebuild)

Once the `gap` image is published to a container registry, users who cannot or do not want to rebuild it locally will be able to pull it directly. For example, after pushing the image under your own namespace, the pull command will look like:

```bash
docker pull cbezaitis/gap
```

- After pulling, all of the commands in the sections above (`docker run --rm gap ...`, MFCC tests, etc.) remain the same, assuming you tag the pulled image locally as `gap`.

FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

# Install OS-level dependencies (from README / getting_started)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        bison \
        build-essential \
        cmake \
        curl \
        doxygen \
        flex \
        git \
        gtkwave \
        libftdi-dev \
        libftdi1 \
        libjpeg-dev \
        libsdl2-dev \
        libsdl2-ttf-dev \
        libsndfile1-dev \
        graphicsmagick-libmagick-dev-compat \
        libtool \
        libusb-1.0-0-dev \
        pkg-config \
        python3-pip \
        rsync \
        scons \
        texinfo \
        wget \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Ensure "python" points to python3
RUN update-alternatives --install /usr/bin/python python /usr/bin/python3 10

# Install the GAP/RISC-V toolchain (Ubuntu 20.04 version as in README)
# The official install script uses sudo; inside the container we are root,
# so we reproduce its rsync logic directly.
RUN git clone https://github.com/GreenWaves-Technologies/gap_riscv_toolchain_ubuntu.git /opt/gap_riscv_toolchain_ubuntu && \
    mkdir -p /usr/lib/gap_riscv_toolchain && \
    rsync -av --delete --exclude ".git*" /opt/gap_riscv_toolchain_ubuntu/ /usr/lib/gap_riscv_toolchain && \
    rm -rf /opt/gap_riscv_toolchain_ubuntu

# Create workspace and copy SDK
WORKDIR /opt/gap_sdk
COPY . .

# Install Python dependencies for SDK and docs
RUN pip3 install --upgrade pip && \
    pip3 install -r requirements.txt && \
    pip3 install -r doc/requirements.txt

# Install extra Python packages used by examples (MFCC, DSP LUT generation, etc.)
# so they are available in the image and do not need to be installed at runtime.
RUN pip3 install --no-cache-dir \
    numpy \
    librosa

# Default to an interactive shell
CMD ["/bin/bash"]


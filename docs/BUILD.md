# Build Instructions

This document describes how to build FiXPro firmware on different platforms.

## Requirements

### Hardware
- Raspberry Pi Pico or compatible RP2040 board
- USB-C cable
- Computer with USB port

### Software

| Software | Version | Notes |
|----------|---------|-------|
| CMake | ≥ 3.13 | Build system |
| ARM GCC | ≥ 10.3 | C/C++ compiler |
| Python | ≥ 3.8 | For CLI tools |
| Pico SDK | 2.0.0 | Bundled or cloned |

### Linux

```bash
# Install ARM toolchain
sudo apt-get update
sudo apt-get install -y \
  gcc-arm-none-eabi binutils-arm-none-eabi \
  cmake make g++ ninja-build \
  libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib

# Clone Pico SDK (if not using bundled)
git clone --depth 1 --branch 2.0.0 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init --depth 1 && cd ..
```

### macOS

```bash
# Install ARM toolchain via Homebrew
brew install --cask gcc-arm-embedded
brew install cmake ninja

# Clone Pico SDK
git clone --depth 1 --branch 2.0.0 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init --depth 1 && cd ..
```

### Windows (WSL2)

```bash
# In WSL2 terminal
sudo apt-get update
sudo apt-get install -y \
  gcc-arm-none-eabi binutils-arm-none-eabi \
  cmake make g++ ninja-build

# Clone Pico SDK
git clone --depth 1 --branch 2.0.0 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk && git submodule update --init --depth 1 && cd ..
```

### Windows (Native)

1. Install [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/)
2. Install [CMake](https://cmake.org/download/)
3. Install [ARM GCC](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm)
4. Use PowerShell or CMD for build commands

## Building Firmware

### Method 1: Automatic (Recommended)

```bash
cd FiXPro
make
```

### Method 2: Manual

```bash
cd firmware

# Create build directory
mkdir -p build && cd build

# Configure CMake
cmake -S . -B build \
  -DPICO_SDK_PATH=/path/to/pico-sdk \
  -DPICO_BOARD=pico \
  -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j$(nproc)
```

### Output Files

After successful build, the following files will be in `firmware/build/`:

| File | Description |
|------|-------------|
| `FiXPro.uf2` | UF2 bootloader format (drag & drop) |
| `FiXPro.bin` | Raw binary |
| `FiXPro.hex` | Intel HEX format |
| `FiXPro.elf` | ELF with debug symbols |

## GitHub Actions

The firmware builds automatically on every push:

```bash
# Push to trigger build
git push origin main

# Create release
git tag v1.0.0
git push origin v1.0.0
```

Build artifacts are available in the Actions tab.

## Troubleshooting

### "CMAKE_C_COMPILER not found"

Ensure ARM toolchain is installed and in PATH:
```bash
arm-none-eabi-gcc --version
```

### " Pico SDK not found"

Set `PICO_SDK_PATH` to the correct SDK location:
```bash
export PICO_SDK_PATH=/path/to/pico-sdk
```

### Build fails with "undefined reference"

Ensure all submodules are initialized:
```bash
cd pico-sdk
git submodule update --init
```

### "Permission denied" when flashing

On Linux, you may need to add udev rules:
```bash
echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="2e8a", MODE="0666"' | sudo tee /etc/udev/rules.d/99-pico.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

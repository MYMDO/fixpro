# SPDX-License-Identifier: GPL-3.0-or-later
#
# FiXPro Build System
# Makefile for building firmware and running CLI
#

SHELL := /bin/bash

.PHONY: all firmware clean flash test install help

# Default target
all: firmware

#==============================================================================
# FIRMWARE BUILD
#==============================================================================

FIRMWARE_DIR := firmware
BUILD_DIR := $(FIRMWARE_DIR)/build
PICO_SDK_PATH ?= $(HOME)/pico-sdk
PICO_SDK_VERSION := 2.0.0

# Check if Pico SDK is available
PICO_SDK_FOUND := $(shell [ -d "$(PICO_SDK_PATH)" ] && echo "yes" || echo "no")

firmware: $(BUILD_DIR)/FiXPro.uf2

$(BUILD_DIR)/FiXPro.uf2: $(FIRMWARE_DIR)/CMakeLists.txt
	@if [ "$(PICO_SDK_FOUND)" = "no" ]; then \
		echo "Pico SDK not found at $(PICO_SDK_PATH)"; \
		echo "Please set PICO_SDK_PATH environment variable or install SDK"; \
		echo ""; \
		echo "To install Pico SDK:"; \
		echo "  git clone --depth 1 --branch $(PICO_SDK_VERSION) https://github.com/raspberrypi/pico-sdk.git $(PICO_SDK_PATH)"; \
		echo "  cd $(PICO_SDK_PATH) && git submodule update --init"; \
		exit 1; \
	fi
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
		cmake -DPICO_SDK_PATH=$(PICO_SDK_PATH) .. && \
		make -j$$(nproc)
	@echo ""
	@echo "Firmware built successfully: $(BUILD_DIR)/FiXPro.uf2"

# Build with specific SDK path
firmware-sdk:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && \
		cmake -DPICO_SDK_PATH=$(PICO_SDK_PATH) .. && \
		make -j$$(nproc)

#==============================================================================
# FLASHING
#==============================================================================

flash: firmware
	@echo "Copy firmware to Pico..."
	@cp $(BUILD_DIR)/FiXPro.uf2 /media/*/RPI-RP2/ 2>/dev/null || \
	cp $(BUILD_DIR)/FiXPro.uf2 /Volumes/RPI-RP2/ 2>/dev/null || \
	echo "Please manually copy $(BUILD_DIR)/FiXPro.uf2 to your Pico"

flash-uf2:
	@echo "Firmware location: $(BUILD_DIR)/FiXPro.uf2"

#==============================================================================
# TESTING
#==============================================================================

test: test-cli

test-cli:
	@echo "Testing CLI..."
	@python3 $(FIRMWARE_DIR)/../host/cli/fixpro.py --help

#==============================================================================
# INSTALLATION
#==============================================================================

install-cli:
	@echo "Installing FiXPro CLI..."
	@install -Dm755 host/cli/fixpro.py /usr/local/bin/fixpro
	@echo "CLI installed to /usr/local/bin/fixpro"
	@echo ""
	@echo "Dependencies required:"
	@echo "  pip install pyserial"

install-deps:
	@echo "Installing Python dependencies..."
	@pip install pyserial --user

#==============================================================================
# CLEANING
#==============================================================================

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directory"

distclean: clean
	@find . -name "*.pyc" -delete
	@find . -name "__pycache__" -type d -delete
	@echo "Distclean complete"

#==============================================================================
# DOCUMENTATION
#==============================================================================

docs:
	@echo "Generating documentation..."
	@mkdir -p docs/html
	@doxygen docs/Doxyfile 2>/dev/null || echo "Doxygen not installed"

#==============================================================================
# HELP
#==============================================================================

help:
	@echo "FiXPro Build System"
	@echo ""
	@echo "Targets:"
	@echo "  firmware       Build firmware (requires Pico SDK)"
	@echo "  flash         Build and flash to Pico"
	@echo "  test          Run tests"
	@echo "  install-cli   Install CLI tool"
	@echo "  install-deps  Install Python dependencies"
	@echo "  clean         Clean build directory"
	@echo "  docs          Generate documentation"
	@echo "  help          Show this help"
	@echo ""
	@echo "Environment variables:"
	@echo "  PICO_SDK_PATH  Path to Pico SDK (default: ~/pico-sdk)"
	@echo ""
	@echo "Requirements:"
	@echo "  - Pico SDK 2.0+"
	@echo "  - CMake 3.13+"
	@echo "  - ARM GCC toolchain"
	@echo "  - Python 3.8+ with pyserial"

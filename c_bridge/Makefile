# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
VCPKG_ROOT = C:/vcpkg
VCPKG_TRIPLET = x64-windows

# vcpkg paths
VCPKG_INCLUDE = $(VCPKG_ROOT)/installed/$(VCPKG_TRIPLET)/include
VCPKG_LIB = $(VCPKG_ROOT)/installed/$(VCPKG_TRIPLET)/lib

# Include and library flags
INCLUDES = -I.. -I$(VCPKG_INCLUDE)
LDFLAGS = -L$(VCPKG_LIB) -lcurl -lcjson

# Source files
SRCDIR = .
SOURCES = $(SRCDIR)/main.c
TARGET = c_bridge.exe

# Default target
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(TARGET) $(SOURCES) $(LDFLAGS)

# Clean target
clean:
	del /f $(TARGET) 2>nul || rm -f $(TARGET)

# Install target (optional)
install: $(TARGET)
	copy $(TARGET) C:\Windows\System32\

# Uninstall target (optional)
uninstall:
	del /f C:\Windows\System32\$(TARGET) 2>nul

# Check dependencies
check-deps:
	@echo "Checking for vcpkg dependencies..."
	@if not exist "$(VCPKG_INCLUDE)\curl\curl.h" (echo Error: curl headers not found in vcpkg. Install with: vcpkg install curl && exit 1)
	@if not exist "$(VCPKG_INCLUDE)\cjson\cJSON.h" (echo Error: cjson headers not found in vcpkg. Install with: vcpkg install cjson && exit 1)
	@if not exist "$(VCPKG_LIB)\libcurl.lib" (echo Error: curl library not found in vcpkg && exit 1)
	@if not exist "$(VCPKG_LIB)\cjson.lib" (echo Error: cjson library not found in vcpkg && exit 1)
	@echo "All vcpkg dependencies found!"

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the C bridge application (default)"
	@echo "  clean      - Remove built files"
	@echo "  install    - Install the binary to C:\Windows\System32\"
	@echo "  uninstall  - Remove the binary from C:\Windows\System32\"
	@echo "  check-deps - Check for required vcpkg dependencies"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Note: This Makefile assumes vcpkg is installed at C:\vcpkg"
	@echo "If your vcpkg is elsewhere, set VCPKG_ROOT variable:"
	@echo "  make VCPKG_ROOT=D:\your\vcpkg\path"

.PHONY: all clean install uninstall check-deps help
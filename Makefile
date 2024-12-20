# Compiler Definitions
CC64 = x86_64-w64-mingw32-gcc
CC32 = i686-w64-mingw32-gcc

# Source Files
SRCS = src/wrapper.c

# Compiler Flags
CFLAGS = -O1 -fno-ident -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -falign-jumps=1 -falign-loops=1 -fwhole-program -mconsole -municode -mno-stack-arg-probe -Xlinker --stack=0x200000,0x200000 -Wall -Wextra -ffreestanding

# Linker Flags
LDFLAGS = -lurlmon -lkernel32 -lucrtbase -nostdlib -lshell32 -lshlwapi -luser32 -s

# Build Directories
BUILD_DIR64 = build/x64
BUILD_DIR32 = build/x86

# Distribution Directories
DIST_ZIP_DIR = dist/zip
DIST_TAR_DIR = dist/tar

# Executable Names
TARGET64 = powershell64.exe
TARGET32 = powershell32.exe

# Distribution Archives
ZIP_ARCHIVE = powershell-wrapper.zip
TAR_ARCHIVE = powershell-wrapper.tar.gz

# Phony Targets
.PHONY: all debug clean dist zip targz

# Default Target
all: $(TARGET32) $(TARGET64)

# Debug Build
debug: CFLAGS += -DENABLE_DEBUG_LOG -g
debug: all

# 64-bit Executable
$(TARGET64): $(SRCS)
	@echo "Building 64-bit executable..."
	$(CC64) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $@

# 32-bit Executable
$(TARGET32): $(SRCS)
	@echo "Building 32-bit executable..."
	$(CC32) $(SRCS) $(CFLAGS) $(LDFLAGS) -o $@

# Distribution Target
dist: zip targz

# Create ZIP Archive
zip: $(TARGET32) $(TARGET64)
	@mkdir -p $(DIST_ZIP_DIR)/32 $(DIST_ZIP_DIR)/64
	@echo "Copying executables and profile.ps1 to ZIP distribution directories..."
	cp $(TARGET32) $(DIST_ZIP_DIR)/32/powershell.exe
	cp $(TARGET64) $(DIST_ZIP_DIR)/64/powershell.exe
	cp src/profile.ps1 $(DIST_ZIP_DIR)/
	@echo "Creating ZIP archive..."
	cd $(DIST_ZIP_DIR) && zip -r ../../$(ZIP_ARCHIVE) profile.ps1 32 64
	@echo "ZIP archive created at $(ZIP_ARCHIVE)"

# Create TAR.GZ Archive
targz: $(TARGET32) $(TARGET64)
	@mkdir -p $(DIST_TAR_DIR)/32 $(DIST_TAR_DIR)/64
	@echo "Copying executables and profile.ps1 to TAR.GZ distribution directories..."
	cp $(TARGET32) $(DIST_TAR_DIR)/32/powershell.exe
	cp $(TARGET64) $(DIST_TAR_DIR)/64/powershell.exe
	cp src/profile.ps1 $(DIST_TAR_DIR)/
	@echo "Creating TAR.GZ archive..."
	tar -czvf $(TAR_ARCHIVE) -C $(DIST_TAR_DIR) profile.ps1 32 64
	@echo "TAR.GZ archive created at $(TAR_ARCHIVE)"

# Clean Build and Distribution Artifacts
clean:
	@echo "Cleaning build and distribution directories..."
	rm -f $(TARGET32) $(TARGET64) $(ZIP_ARCHIVE) $(TAR_ARCHIVE)
	rm -rf build dist
	@echo "Clean complete."


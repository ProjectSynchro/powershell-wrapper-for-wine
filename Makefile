# Compiler Definitions
GO = go

# Source Files
SRCS = src/wrapper.go

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
debug: all

# 64-bit Executable
$(TARGET64): $(SRCS)
	@echo "Building 64-bit executable..."
	GOOS=windows GOARCH=amd64 $(GO) build -x -o $(TARGET64) $(SRCS)

# 32-bit Executable
$(TARGET32): $(SRCS)
	@echo "Building 32-bit executable..."
	GOOS=windows GOARCH=386 $(GO) build -x -o $(TARGET32) $(SRCS)

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


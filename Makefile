# Set common CFLAGS
CFLAGS=-O1 -fno-ident -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -falign-jumps=1 -falign-loops=1 -fwhole-program -mconsole -municode -mno-stack-arg-probe -Xlinker --stack=0x200000,0x200000 -nostdlib  -Wall -Wextra -ffreestanding -lurlmon -lkernel32 -lucrtbase -nostdlib -lshell32 -lshlwapi -s

all: powershell32.exe powershell64.exe

powershell64.exe: src/wrapper.c
	x86_64-w64-mingw32-gcc $^ $(CFLAGS) -o $@

powershell32.exe: src/wrapper.c
	i686-w64-mingw32-gcc $^ $(CFLAGS) -o $@

dist: zip targz

zip: powershell32.exe powershell64.exe
	mkdir -p dist/zip/32 dist/zip/64
	cp powershell32.exe dist/zip/32/powershell.exe
	cp powershell64.exe dist/zip/64/powershell.exe
	cp src/profile.ps1 dist/zip/
	cd dist/zip && zip -r ../../powershell-wrapper.zip profile.ps1 32 64

targz: powershell32.exe powershell64.exe
	mkdir -p dist/tar/32 dist/tar/64
	cp powershell32.exe dist/tar/32/powershell.exe
	cp powershell64.exe dist/tar/64/powershell.exe
	cp src/profile.ps1 dist/tar/
	tar -czvf powershell-wrapper.tar.gz -C dist/tar profile.ps1 32 64

clean:
	rm -f powershell32.exe powershell64.exe powershell-wrapper.zip powershell-wrapper.tar.gz
	rm -rf dist

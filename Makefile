# Set common CFLAGS
CFLAGS=-O1 -fno-ident -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -falign-jumps=1 -falign-loops=1 -fwhole-program -mconsole -municode -mno-stack-arg-probe -Xlinker --stack=0x200000,0x200000 -nostdlib  -Wall -Wextra -ffreestanding -lurlmon -lkernel32 -lucrtbase -nostdlib -lshell32 -lshlwapi -s

all: powershell32.exe powershell64.exe

powershell64.exe: src/wrapper.c
	x86_64-w64-mingw32-gcc -c $(CPPFLAGS) $(CFLAGS) $^ -o $@

powershell32.exe: src/wrapper.c
	i686-w64-mingw32-gcc -c $(CPPFLAGS) $(CFLAGS) $^ -o $@
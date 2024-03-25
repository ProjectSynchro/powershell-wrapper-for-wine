# powershell-wrapper for wine

Wrapper for powershell.exe from wine ( this gets for example Waves Central in wine going ).

Uses pwsh.exe from Powershell Core to get functionality for powershell.exe in wine:

Various commands fed to powershell.exe have a slightly different syntax then what pwsh.exe understands, so these commands are rewritten so pwsh.exe "understands" them.
For example 'powershell.exe -Nologo 1+2' is internally reworked to 'pwsh.exe -Nologo -c 1+2'

If the command is still incompatible with pwsh.exe there's an option to replace (parts of) the command to fix things (in profile.ps1).
See profile.ps1 for an example: the ambigous command 'measure -s' (for which pwsh will throw an error) is replaced with 'measure -sum'

For fun I changed code from standard main(argc,*argv[]) to something like (this)[https://nullprogram.com/blog/2016/01/31/]
# Compiling

You'll need a basic mingw toolchain, for Fedora see: https://fedoraproject.org/wiki/MinGW/Tutorial 

- cd into teh cloned git repository

- Compile the 32 and 64 bit binaries with teh following:

```mingw64-gcc -O1 -fno-ident -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -falign-jumps=1 -falign-loops=1 -fwhole-program -mconsole -municode -mno-stack-arg-probe -Xlinker --stack=0x200000,0x200000 -nostdlib  -Wall -Wextra -ffreestanding  main.c -lurlmon -lkernel32 -lucrtbase -nostdlib -lshell32 -lshlwapi -s -o powershell64.exe
 ```

```mingw32-gcc -O1 -fno-ident -fno-stack-protector -fomit-frame-pointer -fno-unwind-tables -fno-asynchronous-unwind-tables -falign-functions=1 -falign-jumps=1 -falign-loops=1 -fwhole-program -mconsole -municode -mno-stack-arg-probe -Xlinker --stack=0x200000,0x200000 -nostdlib  -Wall -Wextra -ffreestanding main.c -lurlmon -lkernel32 -lucrtbase -nostdlib -lshell32 -lshlwapi -s -o powershell32.exe
 ```

# Install 

**TODO** 

Powershell Core (and ConEmu) are downloaded and installed at first invokation of powershell (i.e. wine powershell`)
(ConEmu is installed to work around bug https://bugs.winehq.org/show_bug.cgi?id=49780)
For an unattended install you could do (thanks brunoais for the tip):

Assuming `~/.wine` is where your wineprefix is
  
```
cp -rf ./powershell64.exe ~/.wine/drive_c/windows/system32/WindowsPowerShell/v1.0/powershell.exe
  
cp -rf ./powershell32.exe ~/.wine/drive_c/windows/syswow64/WindowsPowerShell/v1.0/powershell.exe
```

- WINEARCH=win32 is _not_ supported (yet)
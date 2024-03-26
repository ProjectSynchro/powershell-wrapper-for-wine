# powershell-wrapper for wine

Wrapper for powershell.exe from wine ( this helps fix the RSI Launcher, when it creates directories. ).

Uses pwsh.exe from Powershell Core to get functionality for powershell.exe in wine:

Various commands fed to powershell.exe have a slightly different syntax then what pwsh.exe understands, so these commands are rewritten so pwsh.exe "understands" them.
For example 'powershell.exe -Nologo 1+2' is internally reworked to 'pwsh.exe -Nologo -c 1+2'

If the command is still incompatible with pwsh.exe there's an option to replace (parts of) the command to fix things (in profile.ps1).
See profile.ps1 for an example: the ambigous command 'measure -s' (for which pwsh will throw an error) is replaced with 'measure -sum'

For fun I changed code from standard main(argc,*argv[]) to something like [this](https://nullprogram.com/blog/2016/01/31/])

# Install 

## Winetricks or Protontricks (Recommended)

- Clone git repository
- Change directories to git repository
- Install the custom winetricks verbs:
   - `winetricks --unattended powershell_core.verb conemu.verb powershell_wrapper.verb`
   - Or you can use protontricks: `protontricks <steamappID> --unattended powershell_core.verb conemu.verb`


# Compiling and Manual Install

You'll need a basic mingw toolchain, for Fedora see: https://fedoraproject.org/wiki/MinGW/Tutorial 

Clone the git repository.

To compile the binaries, you can run `make`

Want to only build one architecture? Try
`make powershell32.exe` or `make powershell64.exe`

Install Powershell Core and ConEmu using Winetricks (Or you can do it manually, however this is far easier.):
- Run the custom winetricks verbs:
`winetricks --unattended powershell_core.verb conemu.verb` or `protontricks <steamappID> --unattended powershell_core.verb conemu.verb`

Assuming `~/.wine` is where your wineprefix is
  
```
cp -rf ./powershell64.exe ~/.wine/drive_c/windows/system32/WindowsPowerShell/v1.0/powershell.exe
cp -rf ./powershell32.exe ~/.wine/drive_c/windows/syswow64/WindowsPowerShell/v1.0/powershell.exe
cp -rf ./src/profile.ps1 "~/.wine/drive_c/Program Files/PowerShell/7/profile.ps1"
```

- WINEARCH=win32 is _not_ supported (yet)
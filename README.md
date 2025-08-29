> ⚠️ **Notice:**  
> This project has migrated to **[Codeberg](https://codeberg.org/Synchro/powershell-wrapper-for-wine)**.  

[![CI Testing](https://github.com/ProjectSynchro/powershell-wrapper-for-wine/actions/workflows/ci.yml/badge.svg)](https://github.com/ProjectSynchro/powershell-wrapper-for-wine/actions/workflows/ci.yml)

[![Powershell Wrapper Release](https://github.com/ProjectSynchro/powershell-wrapper-for-wine/actions/workflows/create_release.yml/badge.svg)](https://github.com/ProjectSynchro/powershell-wrapper-for-wine/actions/workflows/create_release.yml)

# powershell-wrapper for wine

Wrapper for powershell.exe from wine ( this helps fix the RSI Launcher, when it creates directories. ).

Uses pwsh.exe from Powershell Core to get functionality for powershell.exe in wine:

Various commands fed to powershell.exe have a slightly different syntax then what pwsh.exe understands, so these commands are rewritten so pwsh.exe "understands" them.
For example 'powershell.exe -Nologo 1+2' is internally reworked to 'pwsh.exe -Nologo -c 1+2'

If the command is still incompatible with pwsh.exe there's an option to replace (parts of) the command to fix things (in profile.ps1).
See profile.ps1 for an example: the ambigous command 'measure -s' (for which pwsh will throw an error) is replaced with 'measure -sum'


# Install 

## Winetricks or Protontricks (Recommended)

- Clone git repository
- Change directories to git repository
- Install the custom winetricks verbs:
   - `winetricks --unattended ./verbs/powershell_core.verb ./verbs/powershell.verb`
   - Or you can use protontricks: 
      - `protontricks <steamappID> --unattended ./verbs/powershell_core.verb ./verbs/powershell.verb`

# Compiling and Manual Install

You'll need golang installed, for Fedora see: https://developer.fedoraproject.org/tech/languages/go/go-installation.html

Clone the git repository.

To compile the binaries, you can run `make`

Want to only build one architecture? Try
`make powershell32.exe` or `make powershell64.exe`

Install Powershell Core using Winetricks (Or you can do it manually, however this is far easier.):
- Run the custom winetricks verbs:
`winetricks --unattended ./verbs/powershell_core.verb` 
or `protontricks <steamappID> --unattended ./verbs/powershell_core.verb`

Assuming `~/.wine` is where your wineprefix is
  
```
cp -rf ./powershell64.exe ~/.wine/drive_c/windows/system32/WindowsPowerShell/v1.0/powershell.exe
cp -rf ./powershell32.exe ~/.wine/drive_c/windows/syswow64/WindowsPowerShell/v1.0/powershell.exe
cp -rf ./src/profile.ps1 "~/.wine/drive_c/Program Files/PowerShell/7/profile.ps1"
```

# powershell-wrapper for wine

**These files are here to be called from a winetricks verb.**
**Make sure you are downloading the latest commits on the master branch, otherwise you will get checksum mismatches.**

## Version Log:
- v0.0.1 - Test for winetricks verb download.
- v1.0.0 - Initial release, built with Github Actions.
- v1.5.0 - Remove dependency on ConEmu
- v2.0.0 - Fix issues in RSI Launcher as well as other electron apps that use sudo-prompt
   - Additionally add support for 32bit wineprefixes
- v2.0.1 - Hotfix a dumb mistake that causes the RSI Launcher to stop working.
- v2.1.0 - Fix support for handling pipeline inputs.
- v2.1.1 - Fix support for non-english and nonstandard "Program Files" locations.
- v2.1.2 - Add new dist zip and tar.gz archives.
   - These are laid out like this: `/profile.ps1, /32/powershell.exe, /64/powershell.exe`
- v2.1.3 - Fix [ProjectSynchro/powershell-wrapper-for-wine#2](https://github.com/ProjectSynchro/powershell-wrapper-for-wine/issues/2)
   - This fixes installing the EAC service in the RSI Launcher.
- v2.2.0 - Add debugging and fix single quotes if they are beside an escaped double quote.
   - This fixes the edge case seen when running the RSI Launcher's installer-support.exe binary.
- v2.2.1 - Fix debugging and revert quote replacement changes.
   - This fixes the edge case seen when running the vcredist binary in the RSI Launcher.
   - The EAC service will fail to install due to how it is invokved, I'll have to rewrite this in something that can use regex a bit better.
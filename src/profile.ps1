#Put workarounds/hacks here.../Adjust to your own needs. It goes into c:\\Program Files\\Powershell\\7\\profile.ps1

#Remove ~/Documents/Powershell/Modules from modulepath; it becomes a mess because it`s not removed when one deletes the wineprefix...
$path = $env:PSModulePath -split ';' ; $env:PSModulePath  = ( $path | Select-Object -Skip 1 | Sort-Object -Unique) -join ';'

# Force plaintext rendering for outputs, and remove PSReadLine for inputs.
# Removes the need for ConEmu, by removing all colours.
# Works around wine bug 49780: https://bugs.winehq.org/show_bug.cgi?id=49780
# See: https://github.com/PowerShell/PSReadLine/issues/3918 and https://github.com/PowerShell/PowerShell/issues/21160 for context.
Remove-Module PSReadLine -Force -ErrorAction SilentlyContinue

#Dry-run 'powershell.exe' once to set env vars below (after you changed them); They are written to HKCU:Environment
#To enable/disable replacing strings in the cmdline fed to pwsh.exe set/unset this env var:
[System.Environment]::SetEnvironmentVariable('PSHACKS','1','User')
#Set strings to replace in the cmdline fed to pwsh by setting following two env vars. Use ¶ as seperator for the strings.
#Failing command: powershell.exe -noLogo -command 'ls -r "C:\windows" | measure -s Length | Select -ExpandProperty Sum'
#Powershell Core needs 'measure -sum' instead of 'measure -s' so we replace it
#Also replace for another failing command: the incompatible ' -noExit Register-WMIEvent ' with harmless ' Write-Host FIXME stub!! '
[System.Environment]::SetEnvironmentVariable('PS_FROM',' measure -s ¶ -noExit Register-WMIEvent ','User')
[System.Environment]::SetEnvironmentVariable('PS_TO',' measure -sum ¶ Write-Host FIXME stub!! ','User')
#End hacks

#Register-WMIEvent not available in PS Core, so for now just change into noop
function Register-WMIEvent
{
    exit 0
}

#Prerequisite: Stuff below requires native dotnet (winetricks -q dotnet48) to be installed, otherwise it will just fail!
#
#Only works as of wine-6.20, see https://bugs.winehq.org/show_bug.cgi?id=51871
#
#Examples of usage:
#$(Get-WmiObject win32_videocontroller).name
Function Get-WmiObject([parameter(mandatory=$true, position = 0, parametersetname = 'class')] [string]$class, `
                       [parameter( position = 1, parametersetname = 'class')][string[]]$property="*", `
                       [string]$computername = "localhost", [string]$namespace = "root\cimv2", `
                       [string]$filter, [parameter(parametersetname = 'query')] [string]$query)
{
    if (!(Test-Path  "HKLM:\Software\Microsoft\.NETFramework\v4.0.30319"))
    {  Add-Type -AssemblyName PresentationCore,PresentationFramework; [System.Windows.MessageBox]::Show('This function requires native .NET48 to be installed!   Do "winetricks -q dotnet48" and try again ','Warning','ok','exclamation'); return}
    if(!$query) {
        $query = "SELECT " +  $($property | Join-String -Separator ",") + " FROM " + $class + (($filter) ? (' where ' + $filter ) : ('')) }

    $searcher = [wmisearcher]$query

    $searcher.scope.path = "\\" + $computername + "\" + $namespace

    return [System.Management.ManagementObjectCollection]$searcher.get()
}

Function Get-CIMInstance ( [parameter(mandatory)] [string]$classname, [string[]]$property="*", [string]$filter)
{
     Get-WMIObject $classname -property $property -filter $filter
}

Set-Alias gwmi Get-WmiObject

# Add a reimplementation of Start-Process to workaround issues with Wine's CMD.
# For some reason (echo %ERRORLEVEL%\) > does not pipe to a file like it should but
# echo %ERRORLEVEL% > does. This cmdlet will check for batch files and rewrite that
# string if it exists.
#
# Fixes the issues seen in the RSI Launcher when it makes use of
# sudo-prompt: https://github.com/jorangreef/sudo-prompt
#
# In theory this should also fix other electron apps with this issue.

function Start-Process {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory=$true, Position=0)]
        [string]$FilePath,

        [string[]]$ArgumentList,

        [System.Management.Automation.PSCredential]$Credential,

        [switch]$LoadUserProfile,

        [switch]$NoNewWindow,

        [switch]$PassThru,

        [string]$RedirectStandardError,

        [string]$RedirectStandardInput,

        [string]$RedirectStandardOutput,

        [switch]$UseNewEnvironment,

        [switch]$Wait,

        [ValidateSet('Normal', 'Hidden', 'Minimized', 'Maximized')]
        [string]$WindowStyle = 'Normal',

        [string]$WorkingDirectory,

        [string]$Verb
    )

    # Function to handle logging only if enabled
    function Write-Message {
        param (
            [string]$Message
        )
        if ($env:LOG_DEBUG) {
            $logFilePath = "$env:USERPROFILE\pwsh_powershell_debug.log"
            $logEntry = "$(Get-Date) - $Message"
            Add-Content -Path $logFilePath -Value $logEntry
        }
    }

    Write-Message "Starting process: $FilePath with arguments: $($ArgumentList -join ' ')"

    try {
        # Check if the file exists at the specified path
        if (-not (Test-Path $FilePath -PathType Leaf)) {
            Write-Message "File not found: $FilePath"
            throw "File not found: $FilePath"
        }

        # Check if the file extension is .bat and rewrite the command if needed
        if ([System.IO.Path]::GetExtension($FilePath).ToLower() -eq '.bat') {
            Write-Message "Batch file detected: $FilePath"

            $fileContent = Get-Content $FilePath -Raw
            if ($fileContent -match '\(echo %ERRORLEVEL%\) >') {
                Write-Message "Rewriting batch file: $FilePath"
                $fileContent = $fileContent -replace '\(echo %ERRORLEVEL%\) >', 'echo %ERRORLEVEL% >'
                Set-Content $FilePath -Value $fileContent
            }
        }

        # Initialize ProcessStartInfo
        $processStartInfo = New-Object System.Diagnostics.ProcessStartInfo
        $processStartInfo.FileName = $FilePath
        $processStartInfo.Arguments = $ArgumentList -join ' '
        $processStartInfo.UseShellExecute = $false

        # Set window style if needed
        switch ($WindowStyle) {
            'Hidden'    { $processStartInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Hidden }
            'Minimized' { $processStartInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Minimized }
            'Maximized' { $processStartInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Maximized }
            default     { $processStartInfo.WindowStyle = [System.Diagnostics.ProcessWindowStyle]::Normal }
        }

        # Set working directory if provided
        if ($WorkingDirectory) {
            Write-Message "Setting working directory: $WorkingDirectory"
            $processStartInfo.WorkingDirectory = $WorkingDirectory
        }

        # Set verb if provided
        if ($Verb) {
            Write-Message "Setting verb: $Verb"
            $processStartInfo.Verb = $Verb
        }

        # Handle input/output redirection
        if ($RedirectStandardOutput) {
            Write-Message "Redirecting standard output to: $RedirectStandardOutput"
            $processStartInfo.RedirectStandardOutput = $true
            $processStartInfo.StandardOutputFileName = $RedirectStandardOutput
        }
        if ($RedirectStandardError) {
            Write-Message "Redirecting standard error to: $RedirectStandardError"
            $processStartInfo.RedirectStandardError = $true
            $processStartInfo.StandardErrorFileName = $RedirectStandardError
        }
        if ($RedirectStandardInput) {
            Write-Message "Redirecting standard input from: $RedirectStandardInput"
            $processStartInfo.RedirectStandardInput = $true
        }

        # If credentials provided, load them
        if ($Credential) {
            Write-Message "Using provided credentials for: $($Credential.UserName)"
            $processStartInfo.UserName = $Credential.UserName
            $password = $Credential.GetNetworkCredential().Password
            $processStartInfo.Password = (ConvertTo-SecureString $password -AsPlainText -Force)
        }

        # UseNewEnvironment flag (whether to inherit or create new environment variables)
        if ($UseNewEnvironment) {
            Write-Message "Clearing environment variables for new process"
            $processStartInfo.EnvironmentVariables.Clear()
        }

        # Start process
        Write-Message "Starting process: $FilePath"
        $process = [System.Diagnostics.Process]::Start($processStartInfo)

        # Log process start success
        if ($process) {
            Write-Message "Process started successfully: $FilePath (PID: $($process.Id))"
        }

        # If PassThru is specified, return the process object
        if ($PassThru) {
            return $process
        }

        # Wait for the process to exit if the Wait switch is specified
        if ($Wait) {
            Write-Message "Waiting for process to exit: $FilePath"
            $process.WaitForExit()

            $exitCode = $process.ExitCode
            Write-Message "Process exited with code: $exitCode"
        }

    } catch {
        Write-Message "Error starting process: $_"
        throw $_
    }
}

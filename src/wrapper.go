package main

import (
	"bufio"
	"fmt"
	"log"
	"os"
	"os/exec"
	"runtime"
	"strings"
)

// Set true to force debugging on if desired
var compileForceDebug string = ""
var forceDebug bool

// Get the correct PowerShell path based on system architecture or PATH
func getPowershellPath() string {
	// First, try to find pwsh.exe in PATH
	pwshPath, err := exec.LookPath("pwsh.exe")
	if err == nil && pwshPath != "" {
		return pwshPath
	}

	// If not found in PATH, search in Program Files directories
	var programFiles string
	if runtime.GOARCH == "amd64" {
		programFiles = os.Getenv("ProgramW6432")
	} else {
		programFiles = os.Getenv("ProgramFiles")
	}
	pwshPath = programFiles + "\\PowerShell\\7\\pwsh.exe"

	if _, err := os.Stat(pwshPath); err == nil {
		return pwshPath
	} else {
		fmt.Fprintln(os.Stderr, "Error: pwsh.exe not found in PATH or Program Files directories.")
		os.Exit(1)
	}

	return "" // This line will never be reached
}

// Check if the option is single or last
func isSingleOrLastOption(opt string) bool {
	lowerOpt := strings.ToLower(opt)
	return (strings.HasPrefix(lowerOpt, "-c") && !strings.HasPrefix(lowerOpt, "-config")) ||
		strings.HasPrefix(lowerOpt, "-n") ||
		strings.HasPrefix(lowerOpt, "-f") ||
		opt == "-" ||
		strings.HasPrefix(lowerOpt, "-enc") ||
		strings.HasPrefix(lowerOpt, "-m") ||
		strings.HasPrefix(lowerOpt, "-s")
}

// Replace incompatible commands/strings in the command line
func replaceIncompatibleCommands(cmdline string) string {
	psHacks := os.Getenv("PSHACKS")
	if psHacks == "" {
		return cmdline
	}

	psFrom := os.Getenv("PS_FROM")
	psTo := os.Getenv("PS_TO")
	if psFrom == "" || psTo == "" {
		return cmdline
	}

	fromTokens := strings.Split(psFrom, "¶")
	toTokens := strings.Split(psTo, "¶")

	if len(fromTokens) != len(toTokens) {
		return cmdline
	}

	for i := range fromTokens {
		cmdline = strings.ReplaceAll(cmdline, fromTokens[i], toTokens[i])
	}

	return cmdline
}

// Parse command-line arguments while preserving quotes
func parseArguments(args []string) ([]string, bool) {
	var cmdline []string
	readFromStdin := false

	// Handle options first
	i := 0
	for i < len(args) && strings.HasPrefix(args[i], "-") {
		if !isSingleOrLastOption(args[i]) {
			i++
		}
		if i >= len(args) {
			break
		}
		if args[i] == "-" {
			if i == len(args)-1 {
				readFromStdin = true
				i++
				continue
			} else {
				fmt.Fprintln(os.Stderr, "Invalid usage")
				os.Exit(1)
			}
		}
		lowerArg := strings.ToLower(args[i])
		if strings.HasPrefix(lowerArg, "-ve") || strings.HasPrefix(lowerArg, "-nop") {
			i++
			continue
		}
		cmdline = append(cmdline, args[i])
		i++
	}

	// Insert '-c' if necessary
	if i < len(args) && (i == 0 || (!strings.HasPrefix(strings.ToLower(args[i-1]), "-c") &&
		!strings.HasPrefix(strings.ToLower(args[i-1]), "-enc") &&
		!strings.HasPrefix(strings.ToLower(args[i-1]), "-f") &&
		!strings.HasPrefix(strings.ToLower(args[i]), "/c"))) {
		cmdline = append(cmdline, "-c")
	}

	// For split arguments that contain spaces, add quotes
	if i < len(args) {
		if len(args[i:]) > 1 {
			var cmd strings.Builder
			for j := i; j < len(args); j++ {
				if j > i {
					cmd.WriteString(" ")
				}
				// Quote arguments that contain spaces and aren't already quoted
				if strings.Contains(args[j], " ") && !strings.HasPrefix(args[j], "\"") {
					cmd.WriteString("\"" + args[j] + "\"")
				} else {
					cmd.WriteString(args[j])
				}
			}
			cmdline = append(cmdline, cmd.String())
		} else {
			cmdline = append(cmdline, args[i])
		}
	}

	return cmdline, readFromStdin
}

// Read from standard input and append to cmdline
func readFromStdinAppend(cmdline []string) []string {
	if len(cmdline) == 0 || cmdline[len(cmdline)-1] != "-c" {
		cmdline = append(cmdline, "-c")
	}
	scanner := bufio.NewScanner(os.Stdin)
	for scanner.Scan() {
		cmdline = append(cmdline, strings.TrimSpace(scanner.Text()))
	}
	if err := scanner.Err(); err != nil {
		fmt.Fprintf(os.Stderr, "Error reading from stdin: %v\n", err)
		os.Exit(1)
	}
	return cmdline
}

// Log command lines if debugging is enabled
func logCommandLines(originalCmd, processedCmd string) {
	if os.Getenv("ENABLE_DEBUG_LOG") == "1" || forceDebug {
		f, err := os.OpenFile("C:\\pwsh_wrapper_debug.log", os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error opening log file: %v\n", err)
			return
		}
		defer f.Close()
		logger := log.New(f, "", log.LstdFlags)
		logger.Printf("Original Command Line: %s\n", originalCmd)
		logger.Printf("Processed Command Line: %s\n", processedCmd)
	}
}

func main() {
	// Check if debugging is forced on at compile-time
	if compileForceDebug != "" {
		forceDebug = true
	} else {
		forceDebug = false
	}

	// Debug logging to print received arguments
	if os.Getenv("ENABLE_DEBUG_LOG") == "1" || forceDebug {
		fmt.Println("Received arguments:")
		for i, arg := range os.Args {
			fmt.Printf("arg[%d]: %s\n", i, arg)
		}
	}

	originalCmd := strings.Join(os.Args[1:], " ")

	pwshPath := getPowershellPath() // Updated function call

	// Set environment variables
	os.Setenv("NO_COLOR", "1")
	// os.Setenv("TERM", "xterm-mono")
	os.Setenv("POWERSHELL_UPDATECHECK", "Off")

	if os.Getenv("ENABLE_DEBUG_LOG") == "1" || forceDebug {
		os.Setenv("LOG_DEBUG", "1")
	}

	args := os.Args[1:]
	cmdline, readFromStdin := parseArguments(args)

	if readFromStdin {
		cmdline = readFromStdinAppend(cmdline)
	}

	// Join command parts into a single string
	command := strings.Join(cmdline, " ")

	// Replace incompatible commands/strings
	command = replaceIncompatibleCommands(command)

	// Log command lines if debugging is enabled
	logCommandLines(originalCmd, command)

	// Execute the command through pwsh.exe
	cmd := exec.Command(pwshPath, cmdline...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	cmd.Stdin = os.Stdin
	err := cmd.Run()
	if err != nil {
		if exitError, ok := err.(*exec.ExitError); ok {
			os.Exit(exitError.ExitCode())
		} else {
			fmt.Fprintf(os.Stderr, "Error executing pwsh.exe: %v\n", err)
			os.Exit(1)
		}
	}
	os.Exit(cmd.ProcessState.ExitCode())
}

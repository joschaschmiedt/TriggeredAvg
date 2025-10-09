# PowerShell build script for Triggered Avg Plugin Tests
# This script builds and runs tests using the Open Ephys main project infrastructure

[CmdletBinding()]
param(
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [switch]$Clean,
    [switch]$SkipTests
)

# Set error handling
$ErrorActionPreference = "Stop"

# Get script directory
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ScriptDir "Build"

Write-Host "===============================================" -ForegroundColor Green
Write-Host "Building Triggered Avg Plugin Tests" -ForegroundColor Green
Write-Host "===============================================" -ForegroundColor Green
Write-Host "Script Directory: $ScriptDir"
Write-Host "Build Directory: $BuildDir"
Write-Host "Configuration: $Configuration"
Write-Host "Platform: $Platform"
Write-Host ""

# # Clean previous build if requested or if build directory exists and we're starting fresh
# if ($Clean -or (Test-Path $BuildDir)) {
#     Write-Host "Cleaning previous build..." -ForegroundColor Yellow
#     if (Test-Path $BuildDir) {
#         Remove-Item $BuildDir -Recurse -Force
#     }
# }

# Create build directory
Write-Host "Creating build directory..." -ForegroundColor Cyan
New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
Set-Location $BuildDir

try {
    Write-Host ""
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host "Configuring CMake..." -ForegroundColor Green
    Write-Host "===============================================" -ForegroundColor Green

    # Configure CMake
    $cmakeArgs = @(
        "..",
        "-G", "Visual Studio 17 2022",
        "-A", $Platform,
        "-DBUILD_TESTS=ON",
        "-DOE_DONT_CHECK_BUILD_PATH=TRUE"
    )

    if ($VerbosePreference -eq 'Continue') {
        $cmakeArgs += "--debug-output"
    }

    Write-Host "Running: cmake $($cmakeArgs -join ' ')" -ForegroundColor Gray
    & cmake @cmakeArgs

    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE"
    }

    Write-Host ""
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host "Building project..." -ForegroundColor Green
    Write-Host "===============================================" -ForegroundColor Green

    # Build the project
    $buildArgs = @(
        "--build", ".",
        "--config", $Configuration,
        "--target", "triggered-avg-tests"
        # "--parallel"
    )

    if ($VerbosePreference -eq 'Continue') {
        $buildArgs += "--verbose"
    }

    Write-Host "Running: cmake $($buildArgs -join ' ')" -ForegroundColor Gray
    & cmake @buildArgs

    if ($LASTEXITCODE -ne 0) {
        throw "Build failed with exit code $LASTEXITCODE"
    }

    # Run tests unless skipped
    if (-not $SkipTests) {
        Write-Host ""
        Write-Host "===============================================" -ForegroundColor Green
        Write-Host "Running tests..." -ForegroundColor Green
        Write-Host "===============================================" -ForegroundColor Green

        $ctestArgs = @(
            "--output-on-failure",
            "--config", $Configuration
        )

        if ($VerbosePreference -eq 'Continue') {
            $ctestArgs += "-V"
        }

        Write-Host "Running: ctest $($ctestArgs -join ' ')" -ForegroundColor Gray
        & ctest @ctestArgs

        $testExitCode = $LASTEXITCODE

        if ($testExitCode -eq 0) {
            Write-Host ""
            Write-Host "All tests passed successfully!" -ForegroundColor Green
        } else {
            Write-Host ""
            Write-Host "WARNING: Some tests failed (exit code: $testExitCode)" -ForegroundColor Yellow
        }
    }

    Write-Host ""
    Write-Host "===============================================" -ForegroundColor Green
    Write-Host "Test Results Summary" -ForegroundColor Green
    Write-Host "===============================================" -ForegroundColor Green

    # Look for test executable in the TestBin directory structure (matches main project pattern)
    $testBinDir = Join-Path $BuildDir "TestBin\triggered-avg\$Configuration"
    $testExecutable = Join-Path $testBinDir "triggered-avg-tests.exe"

    Write-Host "Test executable location: $testExecutable"
    Write-Host "Test directory: $testBinDir"

    if (Test-Path $testExecutable) {
        Write-Host "Test executable exists and is ready to run!" -ForegroundColor Green

        # Check for required DLLs
        $requiredDlls = @("gui_testable_source.dll", "test_helpers.dll")
        $missingDlls = @()

        foreach ($dll in $requiredDlls) {
            $dllPath = Join-Path $testBinDir $dll
            if (Test-Path $dllPath) {
                Write-Host "  ✓ Found: $dll" -ForegroundColor Green
            } else {
                Write-Host "  ✗ Missing: $dll" -ForegroundColor Red
                $missingDlls += $dll
            }
        }

        if ($missingDlls.Count -gt 0) {
            Write-Host ""
            Write-Host "Warning: Missing required DLLs. Checking common directory..." -ForegroundColor Yellow
            $commonDir = Join-Path $BuildDir "TestBin\common\$Configuration"
            Write-Host "Common DLL directory: $commonDir"

            if (Test-Path $commonDir) {
                $commonDlls = Get-ChildItem $commonDir -Filter "*.dll" | Select-Object -ExpandProperty Name
                Write-Host "Available DLLs in common: $($commonDlls -join ', ')"

                # Suggest manual copy if DLLs exist in common
                foreach ($dll in $missingDlls) {
                    $sourceDll = Join-Path $commonDir $dll
                    if (Test-Path $sourceDll) {
                        Write-Host "  Found $dll in common directory. Consider updating CMakeLists.txt to copy automatically." -ForegroundColor Yellow
                    }
                }
            } else {
                Write-Host "Common directory not found: $commonDir" -ForegroundColor Red
            }
        }
    } else {
        Write-Host "Warning: Test executable not found at expected location" -ForegroundColor Yellow

        # Look for it in the old location for debugging
        $oldTestExecutable = Join-Path $BuildDir "$Configuration\triggered-avg-tests.exe"
        if (Test-Path $oldTestExecutable) {
            Write-Host "Found test executable in old location: $oldTestExecutable" -ForegroundColor Yellow
            Write-Host "Consider updating CMakeLists.txt to use TestBin directory structure." -ForegroundColor Yellow
            $testExecutable = $oldTestExecutable
        } else {
            Write-Host "Test executable not found in old location either: $oldTestExecutable" -ForegroundColor Red
        }
    }

    # Ask if user wants to run tests manually
    if (-not $SkipTests -and (Test-Path $testExecutable)) {
        Write-Host ""
        $response = Read-Host "Do you want to run tests manually for detailed output? (y/N)"
        if ($response -eq 'y' -or $response -eq 'Y') {
            Write-Host ""
            Write-Host "Running tests manually..." -ForegroundColor Cyan
            Write-Host "Executing: $testExecutable" -ForegroundColor Gray

            # Change to test directory so DLLs can be found
            $originalLocation = Get-Location
            try {
                if (Test-Path (Split-Path $testExecutable)) {
                    Set-Location (Split-Path $testExecutable)
                }
                & $testExecutable
            } finally {
                Set-Location $originalLocation
            }
        }
    }

    Write-Host ""
    Write-Host "Build and test process completed successfully!" -ForegroundColor Green
    Write-Host ""
    Write-Host "To run tests again later:" -ForegroundColor Cyan
    Write-Host "  Set-Location '$BuildDir'"
    Write-Host "  ctest --config $Configuration"
    Write-Host "  OR"
    Write-Host "  Set-Location '$(Split-Path $testExecutable)'"
    Write-Host "  & '$(Split-Path $testExecutable -Leaf)'"
    Write-Host ""

} catch {
    Write-Host ""
    Write-Host "ERROR: $($_.Exception.Message)" -ForegroundColor Red
    Write-Host ""
    Write-Host "Build failed. Check the output above for details." -ForegroundColor Red
    exit 1
} finally {
    # Return to original directory
    Set-Location $ScriptDir
}

# Pause equivalent for PowerShell
Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

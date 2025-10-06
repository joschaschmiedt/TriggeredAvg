@echo off
REM Run tests for the Triggered Average plugin
REM This script helps run the tests with proper error handling

echo Running Triggered Average Plugin Tests...
echo.

REM Change to the build directory
cd /d "%~dp0..\build"

REM Check if the test executable exists
if not exist "Debug\triggered-avg_tests.exe" (
    echo Error: Test executable not found. Please build the tests first:
    echo   cmake -B build -S . -DBUILD_TESTS=ON
    echo   cmake --build build --config Debug --target triggered-avg_tests
    echo.
    pause
    exit /b 1
)

REM Run the tests
echo Executing tests...
echo.
Debug\triggered-avg_tests.exe

REM Check the result
if %ERRORLEVEL% EQU 0 (
    echo.
    echo All tests passed successfully!
) else (
    echo.
    echo Some tests failed. Error code: %ERRORLEVEL%
)

echo.
pause
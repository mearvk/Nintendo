$ErrorActionPreference = "Stop"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Here
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  throw "CMake is required. Install Visual Studio 2019/2022 with the Desktop development with C++ workload, or install CMake and a supported MSVC toolchain."
}
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
  throw "Python 3 is required for the portable native smoke tests."
}
cmake -S . -B build-cmake
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed ($LASTEXITCODE)." }
cmake --build build-cmake --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Native C/C++ build failed ($LASTEXITCODE)." }
python ..\scripts\native_tooling_smoke.py build-cmake\Release
if ($LASTEXITCODE -ne 0) { throw "Native smoke tests failed ($LASTEXITCODE)." }
Write-Host "PASS: all twelve native C/C++ tools built and passed synthetic-fixture smoke tests."

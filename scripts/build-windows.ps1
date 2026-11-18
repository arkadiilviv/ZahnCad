param(
    [string]$Toolchain = "$PSScriptRoot\..\build\toolchain\msys64\ucrt64",
    [string]$BuildDirectory = "$PSScriptRoot\..\build-desktop"
)
$ErrorActionPreference = 'Stop'
$Toolchain = (Resolve-Path -LiteralPath $Toolchain).Path
$source = (Resolve-Path -LiteralPath "$PSScriptRoot\..").Path
$previousPath = $env:PATH
try {
    $env:PATH = "$Toolchain\bin;$previousPath"
    & "$Toolchain\bin\cmake.exe" -S $source -B $BuildDirectory -G Ninja "-DCMAKE_PREFIX_PATH=$Toolchain" -DCMAKE_BUILD_TYPE=Release -DZAHNCAD_DESKTOP=ON -DBUILD_TESTING=ON
    if ($LASTEXITCODE) { throw 'CMake configuration failed' }
    & "$Toolchain\bin\cmake.exe" --build $BuildDirectory --parallel 4
    if ($LASTEXITCODE) { throw 'Compilation failed' }
    & "$Toolchain\bin\ctest.exe" --test-dir $BuildDirectory --output-on-failure
    if ($LASTEXITCODE) { throw 'Tests failed' }
} finally {
    $env:PATH = $previousPath
}

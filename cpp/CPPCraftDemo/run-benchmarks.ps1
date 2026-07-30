$ErrorActionPreference = "Stop"

# Locate a complete Visual Studio C++ Build Tools installation instead of
# depending on cl.exe being available in the caller's PowerShell PATH.
$vswhere = Join-Path ${env:ProgramFiles(x86)} `
    "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswhere)) {
    throw "Visual Studio Installer could not be found"
}

$installationPath = & $vswhere `
    -latest `
    -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $installationPath) {
    throw "Visual Studio C++ Build Tools could not be found"
}

# VsDevCmd defines INCLUDE, LIB, PATH, and the other variables required by
# cl.exe and link.exe. Import its environment into this PowerShell process.
$developerCommand = Join-Path $installationPath `
    "Common7\Tools\VsDevCmd.bat"
$commandLine = `
    "call `"$developerCommand`" -arch=x64 -host_arch=x64 >nul && set"
$environmentLines = & cmd.exe /d /c $commandLine
if ($LASTEXITCODE -ne 0) {
    throw "Unable to initialize the Visual Studio build environment"
}

foreach ($line in $environmentLines) {
    $separator = $line.IndexOf('=')
    if ($separator -le 0) {
        continue
    }

    $name = $line.Substring(0, $separator)
    $value = $line.Substring($separator + 1)
    Set-Item -Path "Env:$name" -Value $value
}

$compiler = (Get-Command cl.exe -ErrorAction Stop).Source
$benchmarkDirectory = $PSScriptRoot
$benchmarkExecutable = Join-Path `
    $benchmarkDirectory `
    "CPPCraftDemoBenchmark.exe"
$resultsPath = Join-Path $benchmarkDirectory "benchmark-results.csv"

# Object files are intermediate products and do not belong in the repository.
$objectDirectory = Join-Path $env:TEMP "CPPCraftDemoBenchmark"
New-Item -ItemType Directory -Path $objectDirectory -Force | Out-Null
$objectDirectoryArgument = $objectDirectory.Replace('\', '/') + '/'

$compilerArguments = @(
    "/nologo",
    "/std:c++17",
    "/EHsc",
    "/O2",
    "/W4",
    "/permissive-",
    (Join-Path $benchmarkDirectory "CPPCraftDemoBenchmark.cpp"),
    (Join-Path $benchmarkDirectory "Benchmark.cpp"),
    (Join-Path $benchmarkDirectory "BenchmarkSuite.cpp"),
    (Join-Path $benchmarkDirectory "QBRecords.cpp"),
    "/Fo:$objectDirectoryArgument",
    "/Fe:$benchmarkExecutable"
)

& $compiler @compilerArguments
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark build failed with exit code $LASTEXITCODE"
}

& $benchmarkExecutable $resultsPath
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark execution failed with exit code $LASTEXITCODE"
}

Write-Host "Results written to $resultsPath"

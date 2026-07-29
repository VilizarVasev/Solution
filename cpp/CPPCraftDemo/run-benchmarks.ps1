param(
    [string]$Compiler = "C:\MinGW\bin\g++.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Compiler)) {
    throw "C++ compiler not found: $Compiler"
}

$benchmarkDirectory = $PSScriptRoot
$benchmarkExecutable = Join-Path $benchmarkDirectory "CPPCraftDemoBenchmark.exe"
$resultsPath = Join-Path $benchmarkDirectory "benchmark-results.csv"

$compilerArguments = @(
    "-std=c++17",
    "-O2",
    "-Wall",
    "-Wextra",
    "-pedantic",
    (Join-Path $benchmarkDirectory "CPPCraftDemoBenchmark.cpp"),
    (Join-Path $benchmarkDirectory "Benchmark.cpp"),
    (Join-Path $benchmarkDirectory "BenchmarkSuite.cpp"),
    (Join-Path $benchmarkDirectory "QBRecords.cpp"),
    "-o",
    $benchmarkExecutable
)

& $Compiler @compilerArguments
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark build failed with exit code $LASTEXITCODE"
}

& $benchmarkExecutable $resultsPath
if ($LASTEXITCODE -ne 0) {
    throw "Benchmark execution failed with exit code $LASTEXITCODE"
}

Write-Host "Results written to $resultsPath"

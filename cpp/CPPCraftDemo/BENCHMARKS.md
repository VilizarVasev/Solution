# Performance benchmarks

The benchmark measures the `QBRecords::FindMatchingRecords` implementation
using databases of 1,000, 10,000, and 100,000 records. String and numeric
searches are measured separately. Numeric columns use indexes, while string
columns retain substring scans.

The iteration count decreases as the database grows so the linear string
scenarios scan approximately the same total number of records. Indexed numeric
scenarios use the same repetition count. The benchmark consumes each returned
collection and records both its final match count and the average time per
search in `benchmark-results.csv`.

Run the benchmark from PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\run-benchmarks.ps1
```

The script discovers Visual Studio C++ Build Tools, initializes its x64 build
environment, compiles in C++17 mode, and then runs the benchmark.

Use the same machine and compiler when comparing results. After changing the
implementation, run the script again and commit the updated CSV together with
the code. Git history then provides the before-and-after comparison.

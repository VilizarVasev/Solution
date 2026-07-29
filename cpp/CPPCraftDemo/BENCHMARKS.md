# Performance benchmarks

The benchmark measures the existing `QBFindMatchingRecords` implementation
using collections of 1,000, 10,000, and 100,000 records. String and numeric
searches are measured separately.

Each configuration scans approximately the same total number of records by
reducing the iteration count as the collection grows. The benchmark validates
the search workload by consuming each returned collection, without assuming
how many records match. It writes the average time per search to
`benchmark-results.csv`.

Run the benchmark from PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\run-benchmarks.ps1
```

Use the same machine and compiler when comparing results. After changing the
implementation, run the script again and commit the updated CSV together with
the code. Git history then provides the before-and-after comparison.

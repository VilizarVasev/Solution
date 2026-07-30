#include "BenchmarkSuite.hpp"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[])
{
    // The runner supplies an absolute path; direct execution uses a local CSV.
    const std::string outputPath =
        argc > 1 ? argv[1] : "benchmark-results.csv";

    try
    {
        // These sizes reveal how search latency scales as the database grows.
        const std::vector<uint32_t> recordCounts =
        {
            1000U,
            10000U,
            100000U
        };

        // String and numeric searches are separate because their comparison
        // and conversion costs differ in QBFindMatchingRecords.
        const std::vector<Benchmark::Scenario> scenarios =
        {
            { "string-hit", "column1", "testdata500" },
            { "numeric-before-delete", "column2", "24" },
            { "id-before-delete", "column0", "524" },
            {
                "delete-id-524",
                "column0",
                "524",
                Benchmark::Operation::DeleteById
            },
            { "id-after-delete", "column0", "524" },
            { "numeric-after-delete", "column2", "24" }
        };

        // The suite applies every scenario to every configured database size.
        BenchmarkSuite suite(recordCounts, scenarios);
        suite.run(outputPath);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Benchmark failed: " << error.what() << std::endl;
        return 1;
    }

    return 0;
}

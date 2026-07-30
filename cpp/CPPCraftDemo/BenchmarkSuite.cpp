#include "BenchmarkSuite.hpp"

#include <algorithm>
#include <stdexcept>

// These constants define work in record checks rather than search calls.
// They allow iteration counts to scale inversely with database size.
const uint32_t BenchmarkSuite::warmupRecordChecks = 1000000U;
const uint32_t BenchmarkSuite::measuredRecordChecks = 10000000U;

BenchmarkSuite::BenchmarkSuite(
    const std::vector<uint32_t>& recordCounts,
    const std::vector<Benchmark::Scenario>& scenarios)
    : recordCounts{ recordCounts },
      scenarios{ scenarios }
{
    // A suite without database sizes or searches cannot produce measurements.
    if (recordCounts.empty())
    {
        throw std::invalid_argument("At least one record count is required");
    }
    if (scenarios.empty())
    {
        throw std::invalid_argument("At least one benchmark scenario is required");
    }
}

void BenchmarkSuite::run(const std::string& outputPath) const
{
    // The first benchmark creates the report and its header. Every following
    // benchmark appends rows for its configured database size.
    bool appendResults = false;
    for (uint32_t recordCount : recordCounts)
    {
        Benchmark benchmark(createConfig(recordCount));
        benchmark.run(scenarios);
        benchmark.writeResults(outputPath, appendResults);
        benchmark.printResults();
        appendResults = true;
    }
}

Benchmark::Config BenchmarkSuite::createConfig(uint32_t recordCount) const
{
    if (recordCount == 0)
    {
        throw std::invalid_argument("Record count must be greater than zero");
    }

    Benchmark::Config config;
    config.recordCount = recordCount;

    // Constant-work formula:
    //     iterations = target record checks / records per search
    //
    // String substring searches still scan recordCount records. For example,
    // measuredRecordChecks=10,000,000 produces 10,000 searches for 1,000
    // records, 1,000 searches for 10,000 records, and 100 searches for 100,000
    // records. Each string configuration therefore checks approximately
    // 10,000,000 records while still reporting time per search. Indexed
    // numeric searches use the same repetition count for a comparable suite
    // sequence, although they no longer scan the entire database.
    // max(1, ...) guarantees that datasets larger than the work target still
    // execute at least one search.
    config.warmupIterations = static_cast<int>(
        std::max<uint32_t>(1, warmupRecordChecks / recordCount));
    config.measuredIterations = static_cast<int>(
        std::max<uint32_t>(1, measuredRecordChecks / recordCount));

    return config;
}

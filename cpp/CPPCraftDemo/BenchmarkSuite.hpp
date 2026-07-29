#pragma once

#include "Benchmark.hpp"

#include <string>
#include <vector>

/**
 * Runs common search scenarios against several database sizes.
 *
 * The suite converts each record count into a Benchmark::Config whose
 * iteration count keeps the amount of scanned data approximately constant.
 */
class BenchmarkSuite
{
public:
    /** Stores the database sizes and scenarios used for every suite run. */
    BenchmarkSuite(
        const std::vector<uint32_t>& recordCounts,
        const std::vector<Benchmark::Scenario>& scenarios);
    ~BenchmarkSuite() = default;

    BenchmarkSuite(const BenchmarkSuite&) = default;
    BenchmarkSuite& operator=(const BenchmarkSuite&) = default;
    BenchmarkSuite(BenchmarkSuite&&) noexcept = default;
    BenchmarkSuite& operator=(BenchmarkSuite&&) noexcept = default;

    /** Runs every database size and writes all results into one CSV file. */
    void run(const std::string& outputPath) const;

private:
    /** Calculates proportional warm-up and measured iteration counts. */
    Benchmark::Config createConfig(uint32_t recordCount) const;

    // Inputs copied at construction so callers may release their vectors.
    std::vector<uint32_t> recordCounts;
    std::vector<Benchmark::Scenario> scenarios;

    // Target record checks keep suite runtime comparable between data sizes.
    static const uint32_t warmupRecordChecks;
    static const uint32_t measuredRecordChecks;
};

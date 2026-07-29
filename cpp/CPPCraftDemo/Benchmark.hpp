#pragma once

#include "QBRecords.h"

#include <cstddef>
#include <string>
#include <vector>

/**
 * Measures QBFindMatchingRecords for one generated database size.
 *
 * A Benchmark instance owns its test records so record creation is excluded
 * from the measured search duration.
 */
class Benchmark
{
public:
    /** Controls the database size and the number of benchmark repetitions. */
    struct Config
    {
        // Number of records generated before any scenario runs.
        uint32_t recordCount = 1000u;

        // Searches used to warm caches; these searches are not timed.
        int warmupIterations = 1000;

        // Searches included in the average search-time calculation.
        int measuredIterations = 10000;
    };

    /** Describes one column and value combination to benchmark. */
    struct Scenario
    {
        // Stable label written to the console and CSV report.
        std::string name;

        // Column passed to QBFindMatchingRecords.
        std::string columnName;

        // Search value passed to QBFindMatchingRecords.
        std::string matchString;
    };

    /** Validates the configuration and generates the configured records. */
    explicit Benchmark(const Config& config);
    ~Benchmark() = default;

    // A benchmark owns mutable results for one run and is not transferred.
    Benchmark(const Benchmark&) = delete;
    Benchmark& operator=(const Benchmark&) = delete;
    Benchmark(Benchmark&&) = delete;
    Benchmark& operator=(Benchmark&&) = delete;

    /** Runs every supplied scenario and replaces results from any prior run. */
    void run(const std::vector<Scenario>& scenarios);

    /** Writes a CSV header when replacing a file, or rows only when appending. */
    void writeResults(const std::string& outputPath, bool append = false) const;

    /** Prints the same measurements in a human-readable form. */
    void printResults() const;

private:
    /** Captures the values reported for one scenario. */
    struct Result
    {
        std::string scenario;
        double averageMilliseconds;
        std::size_t matchesFound;
    };

    /** Warms up and measures one scenario against this instance's records. */
    Result runScenario(const Scenario& scenario) const;

    // Configuration and generated data remain unchanged during a run.
    Config config;
    QBRecordCollection records;

    // Results are populated by run() and consumed by both report methods.
    std::vector<Result> results;
};

#pragma once

#include "QBRecords.h"

#include <cstddef>
#include <string>
#include <vector>

/**
 * Measures QBRecords searches for one generated database size.
 *
 * A Benchmark instance owns its test records so record creation is excluded
 * from the measured search duration.
 */
class Benchmark
{
public:
    /** Identifies whether a scenario searches or changes the collection. */
    enum class Operation
    {
        Find,
        DeleteById
    };

    /** Controls the database size and the number of benchmark repetitions. */
    struct Config
    {
        // Number of records generated before any scenario runs.
        uint32_t recordCount {1000u};

        // Searches used to warm caches; these searches are not timed.
        int warmupIterations {1000};

        // Searches included in the average search-time calculation.
        int measuredIterations {10000};
    };

    /** Describes one column and value combination to benchmark. */
    struct Scenario
    {
        // Stable label written to the console and CSV report.
        std::string name;

        // Pre-resolved column passed to QBRecords::FindMatchingRecords.
        QBRecords::Column column;

        // Search value passed to QBRecords::FindMatchingRecords.
        std::string matchString;

        // Search is the default; delete steps use matchString as a column0 ID.
        Operation operation {Operation::Find};
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
        Operation operation;
        std::string columnName;
        std::string matchValue;
        std::size_t recordCount;
        int iterations;
        double averageMilliseconds;
        std::size_t matchesFound;

        // Relevant only to DeleteById results. A missing ID is a valid no-op.
        bool recordDeleted;
    };

    /** Executes a changing step and returns a non-timed operation result. */
    Result deleteRecord(const Scenario& scenario);

    /** Warms up and measures one scenario against this instance's records. */
    Result measureFindRecords(const Scenario& scenario) const;

    // The database owns its collection and numeric inverted indexes. It is a
    // private benchmark detail so scenarios can only interact through the
    // measured find and delete operations.
    Config config;
    QBRecords records;

    // Results are populated by run() and consumed by both report methods.
    std::vector<Result> results;
};

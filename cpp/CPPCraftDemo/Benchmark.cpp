#include "Benchmark.hpp"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

Benchmark::Benchmark(const Config& benchmarkConfig)
    : config{ benchmarkConfig },
      records{ benchmarkConfig.recordCount }
{
    // Zero or negative values would produce empty data or invalid averages.
    if (config.recordCount == 0)
    {
        throw std::invalid_argument("Record count must be greater than zero");
    }
    if (config.warmupIterations <= 0)
    {
        throw std::invalid_argument("Warm-up iterations must be greater than zero");
    }
    if (config.measuredIterations <= 0)
    {
        throw std::invalid_argument("Measured iterations must be greater than zero");
    }

    // Generate data once so setup time is not included in search timings.
    records.PopulateDummyData("testdata");
}

void Benchmark::run(const std::vector<Scenario>& scenarios)
{
    // A second call represents a fresh run rather than accumulating old rows.
    results.clear();
    for (const auto& scenario : scenarios)
    {
        if (scenario.operation == Operation::DeleteById)
        {
            results.push_back(deleteRecord(scenario));
            continue;
        }

        results.push_back(measureFindRecords(scenario));
    }
}

Benchmark::Result Benchmark::deleteRecord(const Scenario& scenario)
{
    if (scenario.columnName != "column0")
    {
        throw std::invalid_argument(
            "DeleteById scenarios must target column0");
    }

    const uint32_t id = std::stoul(scenario.matchString);
    if (!records.DeleteRecordById(id))
    {
        throw std::runtime_error(
            scenario.name + " could not find the requested record ID");
    }

    // Deletion is performed once between measured searches. Zero iterations
    // and blank measurement columns distinguish it from benchmarked finds.
    return
    {
        scenario.name,
        scenario.operation,
        scenario.columnName,
        scenario.matchString,
        records.Size(),
        0,
        0.0,
        0
    };
}

Benchmark::Result Benchmark::measureFindRecords(
    const Scenario& scenario) const
{
    // Warm-up searches prime code and CPU caches. The volatile accumulator
    // ensures an optimizing compiler cannot discard their returned results.
    volatile std::size_t warmupMatches = 0;
    for (int iteration = 0; iteration < config.warmupIterations; ++iteration)
    {
        warmupMatches += records.FindMatchingRecords(
            scenario.columnName,
            scenario.matchString).size();
    }

    // matchesFound records the cardinality of the final measured search.
    // It is independent of the number of benchmark repetitions.
    std::size_t matchesFound {0};

    // Every result size is consumed through a volatile sink. This prevents
    // dead-code elimination without adding the accumulated count to reports.
    volatile std::size_t resultCountSink {0};

    // Only repeated searches are inside this measurement interval.
    auto start = std::chrono::steady_clock::now();
    for (int iteration = 0; iteration < config.measuredIterations; ++iteration)
    {
        const QBRecords::RecordCollection matchingRecords =
            records.FindMatchingRecords(
                scenario.columnName,
                scenario.matchString);
        matchesFound = matchingRecords.size();
        resultCountSink += matchesFound;
    }
    auto elapsed = std::chrono::steady_clock::now() - start;

    auto elapsedMilliseconds =
        std::chrono::duration<double, std::milli>(elapsed).count();

    // Formula:
    // average milliseconds per search = total milliseconds / search count.
    // Repetition improves timing stability while this division keeps the
    // reported value representative of one FindMatchingRecords call.
    return
    {
        scenario.name,
        scenario.operation,
        scenario.columnName,
        scenario.matchString,
        records.Size(),
        config.measuredIterations,
        elapsedMilliseconds / config.measuredIterations,
        matchesFound
    };
}

void Benchmark::writeResults(const std::string& outputPath, bool append) const
{
    std::ofstream output;

    // The first database size replaces the old report. Later sizes append
    // rows to the same file so the CSV contains only one header.
    if (append)
    {
        output.open(outputPath, std::ios::app);
    }
    else
    {
        output.open(outputPath, std::ios::trunc);
    }

    if (!output)
    {
        throw std::runtime_error("Unable to write benchmark results: " + outputPath);
    }

    if (!append)
    {
        output << "scenario,operation,column,match_value,records,iterations,average_ms,matches_found\n";
    }
    else
    {
        // Visually separate each database-size test sequence in the report.
        output << '\n';
    }

    // Microsecond-level precision expressed in milliseconds keeps very fast
    // searches distinguishable while retaining a single reporting unit.
    output << std::fixed << std::setprecision(6);
    for (const auto& result : results)
    {
        output << result.scenario << ',';

        if (result.operation == Operation::DeleteById)
        {
            output << "delete,"
                   << result.columnName << ','
                   << result.matchValue << ','
                   << result.recordCount << ','
                   << result.iterations << ",,\n";
            continue;
        }

        output << "find,"
               << result.columnName << ','
               << result.matchValue << ','
               << result.recordCount << ','
               << result.iterations << ','
               << result.averageMilliseconds << ','
               << result.matchesFound << '\n';
    }
}

void Benchmark::printResults() const
{
    for (const auto& result : results)
    {
        if (result.operation == Operation::DeleteById)
        {
            std::cout << result.scenario << ": deleted "
                      << result.columnName << '=' << result.matchValue
                      << ", records: " << result.recordCount
                      << std::endl;
            continue;
        }

        std::cout << result.scenario << ": "
                  << std::fixed << std::setprecision(6)
                  << result.averageMilliseconds << " ms/search, matches found: "
                  << result.matchesFound
                  << std::endl;
    }
}

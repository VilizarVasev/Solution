#pragma once

#include <cstdint>
#include <string>
#include <vector>

using std::uint32_t;

/** Represents one row in the in-memory record collection. */
struct QBRecord
{
    uint32_t column0; // Unique record identifier.
    std::string column1;
    long column2;
    std::string column3;
};

/** Contiguous storage used by the baseline in-memory database. */
typedef std::vector<QBRecord> QBRecordCollection;

/**
 * Returns every record whose selected column matches matchString.
 * Numeric columns use equality; string columns use substring matching.
 */
QBRecordCollection QBFindMatchingRecords(
    const QBRecordCollection& records,
    const std::string& columnName,
    const std::string& matchString);

/** Creates deterministic records used by the demo and performance suite. */
QBRecordCollection populateDummyData(
    const std::string& prefix,
    uint32_t numRecords);

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

/**
 * Deletes the record with the unique column0 ID from the same collection.
 *
 * Returns true when a record is deleted and false when the ID is not found.
 * The last record replaces the deleted record, so collection order is not
 * preserved.
 */
bool DeleteRecordByID(
    QBRecordCollection& records,
    uint32_t id);

/** Creates deterministic records used by the demo and performance suite. */
QBRecordCollection populateDummyData(
    const std::string& prefix,
    uint32_t numRecords);

#include "QBRecords.h"

#include <algorithm>

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString)
{
    // This is the baseline. std::copy_if performs a
    // complete linear scan and copies every matching record into result.
    QBRecordCollection result;

    // Search the records for the matching value.
    std::copy_if(records.begin(), records.end(), std::back_inserter(result), [&](QBRecord rec)
    {
        if (columnName == "column0")
        {
            // Convert numeric string to integer value.
            uint32_t matchValue = std::stoul(matchString);
            return matchValue == rec.column0;
        }
        else if (columnName == "column1")
        {
            // Find mathing string in the column
            return rec.column1.find(matchString) != std::string::npos;
        }
        else if (columnName == "column2")
        {
            // column2 is signed, so its query is converted with std::stol.
            long matchValue = std::stol(matchString);
            return matchValue == rec.column2;
        }
        else if (columnName == "column3")
        {
            // Find mathing string in the column
            return rec.column3.find(matchString) != std::string::npos;
        }
        else
        {
            // Unknown column names produce an empty result collection.
            return false;
        }
    });
    return result;
}

QBRecordCollection populateDummyData(const std::string& prefix, uint32_t numRecords)
{
    QBRecordCollection data;

    // Reserve the final capacity so data generation does not repeatedly grow
    // and reallocate the underlying vector.
    data.reserve(numRecords);
    for (uint32_t i = 0; i < numRecords; i++)
    {
        // Deterministic generation rules:
        //   column0 = unique sequential ID
        //   column1 = prefix followed by ID
        //   column2 = ID modulo 100, producing repeating values 0..99
        //   column3 = ID followed by prefix
        //
        // The modulo formula supplies predictable numeric data without sorting
        // assumptions: column2 = i % 100.
        QBRecord rec = { i, prefix + std::to_string(i), static_cast<long>(i % 100), std::to_string(i) + prefix };
        data.emplace_back(rec);
    }
    return data;
}

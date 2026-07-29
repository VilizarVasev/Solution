#include "QBRecords.h"

#include <algorithm>
#include <iterator>
#include <unordered_map>

/** Centralizes the full scan and copy operation shared by non-unique columns. */
template <typename Predicate>
QBRecordCollection copyMatchingRecords(
    const QBRecordCollection& records,
    Predicate predicate)
{
    QBRecordCollection result;
    std::copy_if(
        records.begin(),
        records.end(),
        std::back_inserter(result),
        predicate);
    return result;
}

/** Finds the unique ID and stops scanning as soon as it is found. */
static QBRecordCollection findByColumn0(
    const QBRecordCollection& records,
    const std::string& matchString)
{
    // Convert once per query rather than once for every visited record.
    const uint32_t matchValue = std::stoul(matchString);
    const auto record = std::find_if(
        records.begin(),
        records.end(),
        [matchValue](const QBRecord& currentRecord)
        {
            return currentRecord.column0 == matchValue;
        });

    QBRecordCollection result;
    if (record != records.end())
    {
        result.push_back(*record);
    }
    return result;
}

/** Finds every record containing the requested substring in column1. */
static QBRecordCollection findByColumn1(
    const QBRecordCollection& records,
    const std::string& matchString)
{
    return copyMatchingRecords(
        records,
        [&matchString](const QBRecord& record)
        {
            return record.column1.find(matchString) != std::string::npos;
        });
}

/** Finds every record whose numeric column2 equals the requested value. */
static QBRecordCollection findByColumn2(
    const QBRecordCollection& records,
    const std::string& matchString)
{
    // Convert once per query rather than once for every visited record.
    const long matchValue = std::stol(matchString);
    return copyMatchingRecords(
        records,
        [matchValue](const QBRecord& record)
        {
            return record.column2 == matchValue;
        });
}

/** Finds every record containing the requested substring in column3. */
static QBRecordCollection findByColumn3(
    const QBRecordCollection& records,
    const std::string& matchString)
{
    return copyMatchingRecords(
        records,
        [&matchString](const QBRecord& record)
        {
            return record.column3.find(matchString) != std::string::npos;
        });
}

QBRecordCollection QBFindMatchingRecords(const QBRecordCollection& records, const std::string& columnName, const std::string& matchString)
{
    // Preserve the baseline behavior for an empty collection: no numeric
    // conversion is attempted because there are no records to inspect.
    if (records.empty())
    {
        return QBRecordCollection();
    }

    // Map a column name directly to its specialized finder. The hash lookup
    // and indirect call happen once per search, never inside the record scan.
    // decltype derives the function-pointer type from findByColumn0, keeping
    // this dispatch table synchronized with the helper function signature.
    using FindFunction = decltype(&findByColumn0);
    static const std::unordered_map<std::string, FindFunction> findFunctions =
    {
        { "column0", findByColumn0 },
        { "column1", findByColumn1 },
        { "column2", findByColumn2 },
        { "column3", findByColumn3 }
    };

    const auto findFunction = findFunctions.find(columnName);
    if (findFunction == findFunctions.end())
    {
        // Unknown column names produce an empty result collection.
        return QBRecordCollection();
    }

    return findFunction->second(records, matchString);
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

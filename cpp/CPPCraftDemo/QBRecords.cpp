#include "QBRecords.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <functional>
#include <stdexcept>
#include <system_error>
#include <utility>

// Static constants definitions
const std::size_t QBRecords::columnCount = 4U;


QBRecords::QBRecords(uint32_t configuredRecordCount)
    : recordCount{ configuredRecordCount }
{
    // An empty configured database cannot execute the benchmark scenarios and
    // would also make the configured population operation meaningless.
    if (recordCount == 0)
    {
        throw std::invalid_argument("Record count must be greater than zero");
    }
}

/** Centralizes the full scan and reference collection used by string columns. */
template <typename Predicate>
QBRecords::RecordReferenceCollection
QBRecords::collectMatchingRecordReferences(
    Predicate predicate) const
{
    // Store read-only references rather than copying complete records and
    // their strings. The result therefore depends on this database's lifetime
    // and must not survive a later population or deletion operation.
    RecordReferenceCollection result;

    // Substring matching still examines every record. Only matching records
    // contribute one pointer-sized reference wrapper to the result vector.
    for (const Record& record : records)
    {
        if (predicate(record))
        {
            result.emplace_back(std::cref(record));
        }
    }
    return result;
}

void QBRecords::PopulateDummyData(std::string_view prefix)
{
    // Rebuilding starts from a clean database so calling this public method a
    // second time cannot leave record positions from an earlier population.
    // Clearing the owning vector invalidates all previously returned record
    // references, as documented by FindMatchingRecords.
    records.clear();
    column0Index.clear();
    column2Index.clear();

    // Reserve final capacities to avoid repeated vector growth and hash-table
    // rehashing while the deterministic database is generated.
    records.reserve(recordCount);
    column0Index.reserve(recordCount);
    column2Index.reserve(100U);

    // Own one copy of the non-owning view while it is reused to construct all
    // generated string columns.
    const std::string prefixString(prefix);
    for (uint32_t id = 0; id < recordCount; ++id)
    {
        // Deterministic generation formulas:
        //   column0 = unique sequential ID
        //   column1 = prefix followed by ID
        //   column2 = ID modulo 100, producing repeating values 0..99
        //   column3 = ID followed by prefix
        //
        // Consequently, a complete group of 100 records contains one of each
        // column2 value. For 1,000 records, searching for 24 finds ten records
        // whose IDs are 24, 124, 224, ..., 924.
        const std::string idString = std::to_string(id);

        // Construct both strings separately because their component order is
        // different. Appending reuses each string's owned storage.
        std::string column1(prefixString);
        column1 += idString;

        std::string column3(idString);
        column3 += prefixString;

        // Move the completed strings into the record to avoid copying their
        // allocated character buffers into the collection.
        records.push_back(
        {
            id,
            std::move(column1),
            static_cast<long>(id % 100U),
            std::move(column3)
        });

        // Index the new record only after insertion establishes its stable
        // vector position. Both maps store this position rather than a copy.
        const std::size_t recordIndex = records.size() - 1U;
        addColumn0Index(recordIndex);
        addColumn2Index(recordIndex);
    }
}

QBRecords::RecordReferenceCollection QBRecords::FindMatchingRecords(
    Column column,
    std::string_view matchString) const
{
    // Preserve the previous behavior for an empty database: there is no match
    // and numeric input is not parsed when no records can be returned.
    if (records.empty())
    {
        return {};
    }

    // Enum values map directly to array positions. This avoids hashing and
    // comparing a column-name string on every repeated benchmark search.
    static constexpr std::array<FindFunction, columnCount> findFunctions =
    {
        &QBRecords::findByColumn0,
        &QBRecords::findByColumn1,
        &QBRecords::findByColumn2,
        &QBRecords::findByColumn3
    };

    const std::size_t columnIndex = static_cast<std::size_t>(column);
    if (columnIndex >= findFunctions.size())
    {
        // Guard against a Column produced by an invalid explicit cast.
        return {};
    }

    // Invoke the selected private member function on this database instance.
    return std::invoke(
        findFunctions[columnIndex],
        *this,
        matchString);
}

std::string_view QBRecords::GetColumnName(Column column) const noexcept
{
    // This table follows the exact order of the Column enum and dispatch table.
    static constexpr std::array<std::string_view, columnCount> columnNames =
    {
        "column0",
        "column1",
        "column2",
        "column3"
    };

    const std::size_t columnIndex = static_cast<std::size_t>(column);
    if (columnIndex >= columnNames.size())
    {
        return "unknown";
    }

    return columnNames[columnIndex];
}

bool QBRecords::DeleteRecordById(uint32_t id)
{
    // Any successful deletion invalidates all earlier search results. The
    // deleted record is destroyed, and swap-and-pop can change which record
    // occupies the deleted vector position.
    // column0 is unique, so its index identifies the target without scanning.
    const auto indexedRecord = column0Index.find(id);
    if (indexedRecord == column0Index.end())
    {
        // A missing unique ID leaves the records and both indexes unchanged.
        return false;
    }

    // Capture all values needed to repair indexes before any record is moved.
    const std::size_t deletedIndex = indexedRecord->second;
    const std::size_t lastIndex = records.size() - 1U;
    const long deletedColumn2 = records[deletedIndex].column2;

    // Remove the deleted record from both indexes before its vector slot is
    // overwritten. Removing a column2 position is itself swap-and-pop within
    // that value's much smaller index bucket.
    column0Index.erase(indexedRecord);
    removeColumn2Index(deletedColumn2, deletedIndex);

    if (deletedIndex != lastIndex)
    {
        // Moving the last record avoids shifting all following vector values.
        // Its two index entries must therefore change from lastIndex to the
        // newly occupied deletedIndex before the old last slot is removed.
        const Record& movedRecord = records[lastIndex];
        column0Index[movedRecord.column0] = deletedIndex;
        updateColumn2Index(
            movedRecord.column2,
            lastIndex,
            deletedIndex);
        records[deletedIndex] = std::move(records[lastIndex]);
    }

    // The deleted value is now either the last record or has been overwritten
    // by the former last record, so removing the last slot completes deletion.
    records.pop_back();
    return true;
}

std::size_t QBRecords::Size() const noexcept
{
    // The vector is the authoritative storage; indexes contain no extra rows.
    return records.size();
}

void QBRecords::addColumn0Index(std::size_t recordIndex)
{
    // column0 is unique, so each ID maps to exactly one vector position.
    column0Index.emplace(records[recordIndex].column0, recordIndex);
}

void QBRecords::addColumn2Index(std::size_t recordIndex)
{
    // column2 repeats every 100 records. Its map value is therefore a bucket
    // containing every vector position that has the same numeric value.
    column2Index[records[recordIndex].column2].push_back(recordIndex);
}

void QBRecords::removeColumn2Index(
    long value,
    std::size_t recordIndex)
{
    // Locate the bucket for the deleted record's numeric value.
    auto bucket = column2Index.find(value);
    if (bucket == column2Index.end())
    {
        // Missing index state indicates an internal synchronization defect,
        // not a normal unsuccessful user search.
        throw std::logic_error("column2 index is inconsistent");
    }

    // A bucket stores positions rather than IDs, so locate the exact vector
    // position that is being deleted.
    auto& indexedPositions = bucket->second;
    const auto indexedPosition = std::find(
        indexedPositions.begin(),
        indexedPositions.end(),
        recordIndex);
    if (indexedPosition == indexedPositions.end())
    {
        throw std::logic_error("column2 record position is missing");
    }

    // Bucket order has no meaning. Replace the removed position with the last
    // bucket entry and pop it to avoid shifting the remaining positions.
    *indexedPosition = indexedPositions.back();
    indexedPositions.pop_back();
    if (indexedPositions.empty())
    {
        // Do not retain map keys that no longer have matching records.
        column2Index.erase(bucket);
    }
}

void QBRecords::updateColumn2Index(
    long value,
    std::size_t oldIndex,
    std::size_t newIndex)
{
    // Find the moved record's numeric bucket before changing its stored
    // vector position from oldIndex to newIndex.
    auto bucket = column2Index.find(value);
    if (bucket == column2Index.end())
    {
        throw std::logic_error("moved record's column2 index is missing");
    }

    // Only one bucket entry represents this particular record position.
    auto indexedPosition = std::find(
        bucket->second.begin(),
        bucket->second.end(),
        oldIndex);
    if (indexedPosition == bucket->second.end())
    {
        throw std::logic_error("moved record's position is missing");
    }

    // The record value is unchanged; only its physical vector position moved.
    *indexedPosition = newIndex;
}

template <typename Integer>
Integer QBRecords::parseNumericValue(std::string_view matchString) const
{
    // from_chars expects a non-empty character range for this numeric query.
    if (matchString.empty())
    {
        throw std::invalid_argument("Numeric search value is empty");
    }

    // Parse directly from string_view storage. The requested Integer type
    // controls signedness and range validation without a runtime variant.
    // This also avoids the temporary std::string required by stol or stoul.
    Integer value{};
    const auto parsedValue = std::from_chars(
        matchString.data(),
        matchString.data() + matchString.size(),
        value);

    if (parsedValue.ec == std::errc::result_out_of_range)
    {
        // The text is numeric but cannot be represented by Integer.
        throw std::out_of_range("Numeric search value is out of range");
    }
    if (parsedValue.ec != std::errc() ||
        parsedValue.ptr != matchString.data() + matchString.size())
    {
        // Require the entire input to be a valid unsigned number. A prefix
        // such as "24abc" must not silently become the numeric value 24.
        throw std::invalid_argument("Numeric search value is invalid");
    }

    // Conversion is performed once per query, before the hash-index lookup.
    return value;
}

QBRecords::RecordReferenceCollection QBRecords::findByColumn0(
    std::string_view matchString) const
{
    // Convert the textual API input once and use the numeric ID as a hash key.
    const uint32_t matchValue =
        parseNumericValue<uint32_t>(matchString);
    const auto indexedRecord = column0Index.find(matchValue);
    if (indexedRecord == column0Index.end())
    {
        // The unique index proves that no record has this ID.
        return {};
    }

    // A column0 lookup can return at most one reference because IDs are unique
    // across the database. cref explicitly preserves read-only access.
    return { std::cref(records[indexedRecord->second]) };
}

QBRecords::RecordReferenceCollection QBRecords::findByColumn1(
    std::string_view matchString) const
{
    // Substring semantics cannot use the exact-value numeric indexes. Scan
    // column1 and reference every record containing matchString anywhere in it.
    return collectMatchingRecordReferences(
        [matchString](const Record& record)
        {
            return std::string_view(record.column1).find(matchString) !=
                std::string_view::npos;
        });
}

QBRecords::RecordReferenceCollection QBRecords::findByColumn2(
    std::string_view matchString) const
{
    // The inverted index returns only positions whose column2 equals the
    // requested value; it avoids scanning unrelated database records.
    const long matchValue = parseNumericValue<long>(matchString);
    const auto indexedRecords = column2Index.find(matchValue);
    if (indexedRecords == column2Index.end())
    {
        return {};
    }

    // Reserve the known number of matches so collecting references cannot
    // repeatedly reallocate the output vector.
    RecordReferenceCollection result;
    result.reserve(indexedRecords->second.size());
    // Convert indexed positions into read-only references. No Record or string
    // object is copied while materializing the public search result.
    for (std::size_t recordIndex : indexedRecords->second)
    {
        result.emplace_back(std::cref(records[recordIndex]));
    }
    return result;
}

QBRecords::RecordReferenceCollection QBRecords::findByColumn3(
    std::string_view matchString) const
{
    // column3 uses the same substring rule as column1 but scans its own field.
    return collectMatchingRecordReferences(
        [matchString](const Record& record)
        {
            return std::string_view(record.column3).find(matchString) !=
                std::string_view::npos;
        });
}

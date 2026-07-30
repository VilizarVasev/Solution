#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using std::uint32_t;
using std::uint8_t;

/**
 * Owns the records and the indexes used to search their numeric columns.
 *
 * column0 has a unique-value index that points to one vector position.
 * column2 has a non-unique index that points to every matching position.
 * String columns retain substring semantics and therefore use linear scans.
 */
class QBRecords
{
public:
    /** Identifies one searchable record column without string dispatch. */
    enum class Column : uint8_t
    {
        Column0,
        Column1,
        Column2,
        Column3
    };

    /** Represents one row owned and returned by this database. */
    struct Record
    {
        uint32_t column0; // Unique record identifier.
        std::string column1;
        long column2;
        std::string column3;
    };

    /** Non-owning, read-only handle to one database record. */
    using RecordReference = std::reference_wrapper<const Record>;

    /** Lightweight non-owning result type returned by record searches. */
    using RecordReferenceCollection = std::vector<RecordReference>;

    /** Stores the final number of records that PopulateDummyData will create. */
    explicit QBRecords(uint32_t recordCount);
    ~QBRecords() = default;

    // One QBRecords object represents one unique mutable database. Copying or
    // moving it could accidentally create a second database with stale index
    // ownership, so those operations are explicitly disabled.
    QBRecords(const QBRecords&) = delete;
    QBRecords& operator=(const QBRecords&) = delete;
    QBRecords(QBRecords&&) = delete;
    QBRecords& operator=(QBRecords&&) = delete;

    /** Generates deterministic records and builds both numeric indexes. */
    void PopulateDummyData(std::string_view prefix);

    /**
     * Returns read-only references matching the requested column and value.
     * Numeric columns use equality; string columns use substring matching.
     *
     * The references avoid copying complete records and their strings. They
     * remain valid only while this QBRecords object exists and until its next
     * change. PopulateDummyData and a successful DeleteRecordById invalidate
     * every result previously returned by this object.
     */
    RecordReferenceCollection FindMatchingRecords(
        Column column,
        std::string_view matchString) const;

    /** Returns the stable report name associated with a column value. */
    std::string_view GetColumnName(Column column) const noexcept;

    /**
     * Deletes the unique column0 ID using index lookup and swap-and-pop.
     * Collection order is not preserved; both indexes remain synchronized.
     */
    bool DeleteRecordById(uint32_t id);

    /** Returns the current number of records after any deletions. */
    std::size_t Size() const noexcept;

private:
    /** Owning contiguous storage for the database itself. */
    using RecordCollection = std::vector<Record>;

    /** Collects non-owning references accepted by a predicate. */
    template <typename Predicate>
    RecordReferenceCollection collectMatchingRecordReferences(
        Predicate predicate) const;

    /** Adds one newly appended record to the unique column0 index. */
    void addColumn0Index(std::size_t recordIndex);

    /** Adds one newly appended record to the non-unique column2 index. */
    void addColumn2Index(std::size_t recordIndex);

    /** Removes one vector position from its column2 index bucket. */
    void removeColumn2Index(long value, std::size_t recordIndex);

    /** Replaces a moved record's old position in its column2 index bucket. */
    void updateColumn2Index(
        long value,
        std::size_t oldIndex,
        std::size_t newIndex);

    /** Parses a query directly into the numeric type required by its column. */
    template <typename Integer>
    Integer parseNumericValue(std::string_view matchString) const;

    /** Uses the unique ID index for an average constant-time lookup. */
    RecordReferenceCollection findByColumn0(
        std::string_view matchString) const;

    /** Scans column1 because it requires substring matching. */
    RecordReferenceCollection findByColumn1(
        std::string_view matchString) const;

    /** Uses the non-unique numeric index to retrieve all matching records. */
    RecordReferenceCollection findByColumn2(
        std::string_view matchString) const;

    /** Scans column3 because it requires substring matching. */
    RecordReferenceCollection findByColumn3(
        std::string_view matchString) const;

    /** Derives the common finder pointer type from one finder declaration. */
    using FindFunction = decltype(&QBRecords::findByColumn0);

    /** Number of valid Column values and entries in each dispatch array. */
    static const std::size_t columnCount;

    // The configured population size is distinct from the current size,
    // which can decrease when records are deleted.
    uint32_t recordCount;
    RecordCollection records;

    // Index values are vector positions rather than record copies. This keeps
    // the source record in one place while providing fast numeric lookups.
    std::unordered_map<uint32_t, std::size_t> column0Index;
    std::unordered_map<long, std::vector<std::size_t>> column2Index;
};

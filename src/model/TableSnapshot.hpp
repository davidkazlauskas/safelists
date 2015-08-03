#ifndef TABLESNAPSHOT_5DK8CM2U
#define TABLESNAPSHOT_5DK8CM2U

#include <sstream>
#include <vector>
#include <string>

struct TableSnapshot {

    TableSnapshot();
    ~TableSnapshot();

    TableSnapshot(TableSnapshot&& other);
    TableSnapshot(const TableSnapshot&) = delete;

    friend struct TableSnapshotBuilder;

private:
    int _bufSize;
    char* _buffer;
};

struct TableSnapshotBuilder {

    TableSnapshotBuilder(int columns,const char** headerNames);

    TableSnapshotBuilder() = delete;
    TableSnapshotBuilder(const TableSnapshotBuilder&) = delete;
    TableSnapshotBuilder(TableSnapshotBuilder&&) = delete;

    void newRow();
    void setValue(int column,const char* value);
    TableSnapshot getSnapshot();

private:
    void writeCurrentRow();

    int _columns;
    int _totalCount;
    std::stringstream _stream;
    std::vector< std::string > _tmp;
};

#endif /* end of include guard: TABLESNAPSHOT_5DK8CM2U */

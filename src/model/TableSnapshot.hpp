#ifndef TABLESNAPSHOT_5DK8CM2U
#define TABLESNAPSHOT_5DK8CM2U

#include <sstream>
#include <vector>
#include <string>

struct TableSnapshot {

private:
    int _bufSize;
    char* _buffer;
};

struct TableSnapshotBuilder {

    TableSnapshotBuilder(int columns);

    TableSnapshotBuilder() = delete;
    TableSnapshotBuilder(const TableSnapshotBuilder&) = delete;
    TableSnapshotBuilder(TableSnapshotBuilder&&) = delete;

    void addRow();

private:
    int _columns;
    std::stringstream _stream;
    std::vector< std::string > _tmp;
};

#endif /* end of include guard: TABLESNAPSHOT_5DK8CM2U */

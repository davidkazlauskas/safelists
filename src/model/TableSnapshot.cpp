
#include "TableSnapshot.hpp"

TableSnapshotBuilder::TableSnapshotBuilder(int columns,char** headerNames) :
    _columns(columns), _totalCount(0)
{
    _tmp.resize(_columns);
}

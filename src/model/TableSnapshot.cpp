
#include "TableSnapshot.hpp"

TableSnapshotBuilder::TableSnapshotBuilder(int columns):
    _columns(columns)
{
    _tmp.resize(_columns);
}

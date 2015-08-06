
#include "SqliteRanger.hpp"

typedef std::lock_guard< std::mutex > LGuard;

SqliteRanger::SqliteRanger(
    const std::weak_ptr< Messageable >& asyncSqlite,
    const UpdateFunction& updateFunction,
    const EmptyFunction& emptyFunction
) : _requestedStart(0),
    _requestedEnd(0),
    _actualStart(0),
    _actualEnd(0),
    _asyncSqlite(asyncSqlite),
    _updateFunction(updateFunction),
    _emptyValueFunction(emptyFunction)
{
}

void SqliteRanger::process() {
}

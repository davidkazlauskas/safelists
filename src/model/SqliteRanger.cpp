
#include "SqliteRanger.hpp"

SqliteRanger::SqliteRanger(
    const std::weak_ptr< Messageable >& asyncSqlite,
    const UpdateFunction& updateFunction,
    const EmptyFunction& emptyFunction
) : _asyncSqlite(asyncSqlite),
    _updateFunction(updateFunction),
    _emptyValueFunction(emptyFunction)
{
}

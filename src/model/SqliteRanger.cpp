
#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"
#include "SqliteRanger.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef std::lock_guard< std::mutex > LGuard;

SqliteRanger::SqliteRanger(
    const std::weak_ptr< Messageable >& asyncSqlite,
    const char* headers,
    const UpdateFunction& updateFunction,
    const EmptyFunction& emptyFunction
) : _requestedStart(0),
    _requestedEnd(0),
    _actualStart(0),
    _actualEnd(0),
    _asyncSqlite(asyncSqlite),
    _headers(headers),
    _updateFunction(updateFunction),
    _emptyValueFunction(emptyFunction)
{
}

void SqliteRanger::process() {
}

#define SNAPSHOT_SIG \
    AsyncSqlite::ExecuteOutSnapshot, \
    std::string, \
    std::vector< std::string >, \
    TableSnapshot

void SqliteRanger::setRange(int start,int end) {
    LGuard guard(_mtx);
    if (start == _requestedStart && end == _requestedEnd) {
        return;
    }

    auto locked = _asyncSqlite.lock();
    if (nullptr == locked) {
        return;
    }

    //char queryBuf[256];
    //sprintf(queryBuf,"")

    //auto msg = SF::vpackPtrCustomWCallback<
        //templatious::VPACK_SYNCED,
        //SNAPSHOT_SIG
    //>(nullptr,);

}

void SqliteRanger::updateRange() {
}

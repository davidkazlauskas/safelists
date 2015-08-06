
#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"
#include "SqliteRanger.hpp"

TEMPLATIOUS_TRIPLET_STD;

typedef std::lock_guard< std::mutex > LGuard;

SqliteRanger::SqliteRanger(
    const std::weak_ptr< Messageable >& asyncSqlite,
    const char* query,
    int columnCount,
    const UpdateFunction& updateFunction,
    const EmptyFunction& emptyFunction
) : _requestedStart(0),
    _requestedEnd(0),
    _actualStart(0),
    _actualEnd(0),
    _asyncSqlite(asyncSqlite),
    _query(query),
    _columnCount(columnCount),
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
    if (start == end) {
        return;
    }

    LGuard guard(_mtx);
    if (start == _requestedStart && end == _requestedEnd) {
        return;
    }

    auto locked = _asyncSqlite.lock();
    if (nullptr == locked) {
        return;
    }

    char queryBuf[512];
    int limit = end - start;
    int offset = start;
    // example: "SELECT Name, Id FROM some LIMIT %d OFFSET %d;
    sprintf(queryBuf,_query.c_str(),limit,offset);

    std::vector< std::string > headers(_columnCount);

    SM::traverse<true>([](int i,std::string& header) {
        char printout[16];
        sprintf(printout,"%d",i);
        header.assign(printout);
    });

    auto msg = SF::vpackPtrCustomWCallback<
        templatious::VPACK_SYNCED,
        SNAPSHOT_SIG
    >(nullptr,queryBuf,std::move(headers),TableSnapshot());

    _requestedStart = start;
    _requestedEnd = end;
}

void SqliteRanger::updateRange() {
}

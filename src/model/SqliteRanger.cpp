
#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"
#include "SqliteRanger.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

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
    TableSnapshot moved;
    {
        LGuard g(_mtx);
        if (_pending.isEmpty()) {
            return;
        }
        moved = std::move(_pending);
    }

}

#define SNAPSHOT_SIG \
    SafeLists::AsyncSqlite::ExecuteOutSnapshot, \
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
    },headers);

    auto selfCpy = _self;
    auto msg = SF::vpackPtrCustomWCallback<
        templatious::VPACK_SYNCED,
        SNAPSHOT_SIG
    >(
    [=](const TEMPLATIOUS_VPCORE< SNAPSHOT_SIG >& out) {
        auto locked = selfCpy.lock();
        if (nullptr == locked) {
            return;
        }

        LGuard guard(locked->_mtx);
        auto& nConst = const_cast< TableSnapshot& >(out.fGet<3>());
        locked->_pending = std::move(nConst);
    },
    nullptr,queryBuf,std::move(headers),TableSnapshot());

    _requestedStart = start;
    _requestedEnd = end;
}

void SqliteRanger::updateRange() {
}

void SqliteRanger::setSelf(const std::weak_ptr< SqliteRanger >& self) {
    _self = self;
    auto locked = _self.lock();
    assert( this == locked.get() );
}

} // end of SafeLists namespace


#include <templatious/FullPack.hpp>

#include "AsyncSqlite.hpp"
#include "SqliteRanger.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

typedef std::lock_guard< std::mutex > LGuard;

struct SqliteRangerImpl {

    static void applyEmptyness(SqliteRanger& ranger) {
        int totalToView = ranger._requestedEnd - ranger._requestedStart;
        if (ranger._actualStart < ranger._requestedEnd
         && ranger._actualEnd >= ranger._requestedStart) {
            // =======    > actual
            //     ====== > requested
            int visibleRows = ranger._actualEnd - ranger._requestedStart;
            int shiftBack = ranger._requestedStart - ranger._actualStart;
            for (int i = 0; i < visibleRows; ++i) {
                ranger._valueMatrix[i] =
                    std::move(ranger._valueMatrix[i + shiftBack]);
            }
            ranger._valueMatrix.resize(visibleRows);
        } else if (ranger._actualStart < ranger._requestedStart
                && ranger._actualEnd >= ranger._requestedEnd)
        {
            // =======    > actual
            //   ===      > requested
            int visibleRows = ranger._requestedEnd - ranger._requestedStart;
            int shiftBack = ranger._requestedStart - ranger._actualStart;
            for (int i = 0; i < visibleRows; ++i) {
                ranger._valueMatrix[i] =
                    std::move(ranger._valueMatrix[i + shiftBack]);
            }
            ranger._valueMatrix.resize(visibleRows);
        } else if (ranger._requestedEnd > ranger._actualStart
                && ranger._actualEnd > ranger._requestedEnd)
        {
            //     ====== > actual
            // =======    > requested
            int visibleRows = ranger._requestedEnd - ranger._actualStart;
            //int shiftFront = ranger._requestedEnd -
        }

    }

    static void reset(SqliteRanger& ranger) {
        int totalToView = ranger._requestedEnd - ranger._requestedStart;
        int diff = totalToView - SA::size(ranger._valueMatrix);
        if (diff > 0) {
            ranger._valueMatrix.resize(totalToView);
            int start = totalToView - diff;
            for (int i = start; i < totalToView; ++i) {
                ranger._valueMatrix[i].resize(ranger._columnCount);
                TEMPLATIOUS_0_TO_N(j,ranger._columnCount) {
                    ranger._emptyValueFunction(i,j,ranger._valueMatrix[i][j]);
                }
            }
        }
    }
};

SqliteRanger::SqliteRanger(
    const std::weak_ptr< Messageable >& asyncSqlite,
    const char* query,
    const char* countQuery,
    int columnCount,
    const UpdateFunction& updateFunction,
    const EmptyFunction& emptyFunction
) : _requestedStart(0),
    _requestedEnd(0),
    _actualStart(0),
    _actualEnd(0),
    _numRows(-1),
    _asyncSqlite(asyncSqlite),
    _query(query),
    _countQuery(countQuery),
    _columnCount(columnCount),
    _updateFunction(updateFunction),
    _emptyValueFunction(emptyFunction),
    _rowsFuture(_rowsPromise.get_future())
{
}

void SqliteRanger::process() {
    TableSnapshot moved;
    int actualStartCpy,actualEndCpy;
    {
        LGuard g(_mtx);
        if (_pending.isEmpty()) {
            return;
        }
        moved = std::move(_pending);
        actualStartCpy = _requestedStart;
        actualEndCpy = _requestedEnd;
    }

    moved.traverse(
        [&](int i,int j,const char* value,const char* header) {
            this->_valueMatrix[i][j] = value;
            this->_updateFunction(i,j,value);
            return true;
        }
    );

    _actualStart = actualStartCpy;
    _actualEnd = actualEndCpy;
}

#define SNAPSHOT_SIG \
    SafeLists::AsyncSqlite::ExecuteOutSnapshot, \
    std::string, \
    std::vector< std::string >, \
    TableSnapshot

void SqliteRanger::setRange(int start,int end) {
    if (start == end || start > end) {
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

    locked->message(msg);

    _requestedStart = start;
    _requestedEnd = end;

    // some other time, when everything's done
    //SqliteRangerImpl::applyEmptyness(*this);
    SqliteRangerImpl::reset(*this);
}

void SqliteRanger::updateRange() {
}

void SqliteRanger::setSelf(const std::weak_ptr< SqliteRanger >& self) {
    _self = self;
    auto locked = _self.lock();
    assert( this == locked.get() );
}

void SqliteRanger::getData(int row,int column,std::string& out) const {
    {
        LGuard guard(_mtx);
        if (_actualStart <= row && _actualEnd > row) {
            out = _valueMatrix[row - _actualStart][column];
            return;
        }
    }
    _emptyValueFunction(row,column,out);
}

int SqliteRanger::numRows() const {
    return _numRows;
}

#define UPDATE_ROWS_SIGNATURE   \
    AsyncSqlite::OutSingleNum,  \
    std::string,                \
    int,                        \
    bool                        \

void SqliteRanger::updateRows() {
    auto locked = _asyncSqlite.lock();
    if (nullptr == locked) {
        return;
    }

    auto selfCpy = _self;
    auto msg = SF::vpackPtrCustomWCallback<
        templatious::VPACK_SYNCED,
        UPDATE_ROWS_SIGNATURE
    >([=](const TEMPLATIOUS_VPCORE<UPDATE_ROWS_SIGNATURE>& core) {
        auto lockedSelf = selfCpy.lock();
        if (nullptr == lockedSelf) {
            return;
        }

        if (core.fGet<3>()) {
            lockedSelf->_numRows = core.fGet<2>();
            //if (!_rowsFuture.valid()) {
                lockedSelf->_rowsPromise.set_value();
            //}
        }
    },nullptr,"SELECT COUNT(*) FROM files;",-1,false);

    locked->message(msg);
}

void SqliteRanger::waitRows() {
    _rowsFuture.wait();
}

std::shared_ptr< SqliteRanger > SqliteRanger::makeRanger(
    const std::weak_ptr< Messageable >& asyncSqlite,
    const char* query,
    const char* countQuery,
    int columnCount,
    const UpdateFunction& updateFunction,
    const EmptyFunction& emptyFunction
) {
    std::unique_ptr< SqliteRanger > rng(
        new SqliteRanger(asyncSqlite,query,countQuery,columnCount,
            updateFunction,emptyFunction)
    );

    std::shared_ptr< SqliteRanger > outRes(rng.release());
    outRes->setSelf(outRes);
    return outRes;
}

} // end of SafeLists namespace

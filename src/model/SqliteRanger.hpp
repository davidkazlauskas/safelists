
#ifndef SQLITERANGER_USA0FKBB
#define SQLITERANGER_USA0FKBB

#include <vector>
#include <mutex>
#include <future>

#include "TableSnapshot.hpp"
#include "AsyncSqlite.hpp"

namespace SafeLists {

struct SqliteRanger {

    typedef std::function< void(int,int,const char*) > UpdateFunction;
    typedef std::function< void(int,int,std::string&) > EmptyFunction;

    SqliteRanger() = delete;
    SqliteRanger(const SqliteRanger&) = delete;
    SqliteRanger(SqliteRanger&&) = delete;

    void process();
    void setRange(int start,int end);
    void updateRange();
    void getData(int row,int column,std::string& out) const;
    int numRows() const;
    int numColumns() const;
    void updateRows();
    void waitRows();

    friend struct SqliteRangerImpl;

    static std::shared_ptr< SqliteRanger > makeRanger(
        const std::weak_ptr< Messageable >& asyncSqlite,
        const char* query,
        const char* countQuery,
        int columnCount,
        const UpdateFunction& updateFunction,
        const EmptyFunction& emptyFunction
    );

private:
    SqliteRanger(
        const std::weak_ptr< Messageable >& asyncSqlite,
        const char* query,
        const char* countQuery,
        int columnCount,
        const UpdateFunction& updateFunction,
        const EmptyFunction& emptyFunction
    );

    void setSelf(const std::weak_ptr< SqliteRanger >& self);

    int _requestedStart;
    int _requestedEnd;
    int _actualStart;
    int _actualEnd;
    int _numRows;

    TableSnapshot _pending;

    std::weak_ptr< Messageable > _asyncSqlite;
    const std::string _query;
    const std::string _countQuery;

    const int _columnCount;
    mutable std::mutex _mtx;
    std::vector< std::vector< std::string > > _valueMatrix;
    UpdateFunction _updateFunction;
    EmptyFunction _emptyValueFunction;
    std::weak_ptr< SqliteRanger > _self;

    std::promise<void> _rowsPromise;
    std::future<void> _rowsFuture;
};

}

#endif /* end of include guard: SQLITERANGER_USA0FKBB */


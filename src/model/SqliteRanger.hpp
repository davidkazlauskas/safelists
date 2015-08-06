
#ifndef SQLITERANGER_USA0FKBB
#define SQLITERANGER_USA0FKBB

#include <vector>
#include <mutex>

#include "TableSnapshot.hpp"
#include "AsyncSqlite.hpp"

namespace SafeLists {

struct SqliteRanger {

    typedef std::function< void(int,int,const char*) > UpdateFunction;
    typedef std::function< void(int,int,std::string&) > EmptyFunction;

    SqliteRanger() = delete;
    SqliteRanger(const SqliteRanger&) = delete;
    SqliteRanger(SqliteRanger&&) = delete;

    SqliteRanger(
        const std::weak_ptr< Messageable >& asyncSqlite,
        const char* query,
        int columnCount,
        const UpdateFunction& updateFunction,
        const EmptyFunction& emptyFunction
    );

    void process();
    void setRange(int start,int end);
    void updateRange();

private:
    int _requestedStart;
    int _requestedEnd;
    int _actualStart;
    int _actualEnd;
    TableSnapshot _pending;
    std::weak_ptr< Messageable > _asyncSqlite;
    const std::string _query;
    const int _columnCount;
    std::mutex _mtx;
    std::vector< std::vector< std::string > > _valueMatrix;
    UpdateFunction _updateFunction;
    EmptyFunction _emptyValueFunction;
};

}

#endif /* end of include guard: SQLITERANGER_USA0FKBB */


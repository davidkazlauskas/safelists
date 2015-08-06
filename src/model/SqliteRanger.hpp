
#ifndef SQLITERANGER_USA0FKBB
#define SQLITERANGER_USA0FKBB

#include <vector>
#include <mutex>

#include "AsyncSqlite.hpp"

struct SqliteRanger {

    typedef std::function< void(int,int,const char*) > UpdateFunction;
    typedef std::function< void(int,int,std::string&) > EmptyFunction;

    SqliteRanger() = delete;
    SqliteRanger(const SqliteRanger&) = delete;
    SqliteRanger(SqliteRanger&&) = delete;

    SqliteRanger(
        const std::weak_ptr< Messageable >& asyncSqlite,
        const UpdateFunction& updateFunction,
        const EmptyFunction& emptyFunction
    );

    void process();

private:
    int _requestedStart;
    int _requestedEnd;
    int _actualStart;
    int _actualEnd;
    std::weak_ptr< Messageable > _asyncSqlite;
    std::mutex _mtx;
    std::vector< std::vector< std::string > > _valueMatrix;
    UpdateFunction _updateFunction;
    EmptyFunction _emptyValueFunction;
};

#endif /* end of include guard: SQLITERANGER_USA0FKBB */


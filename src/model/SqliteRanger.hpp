
#ifndef SQLITERANGER_USA0FKBB
#define SQLITERANGER_USA0FKBB

#include <vector>

#include "AsyncSqlite.hpp"

struct SqliteRanger {

private:
    int _requestedStart;
    int _requestedEnd;
    int _actualStart;
    int _actualEnd;
    std::vector< std::vector< std::string > > _valueMatrix;
    std::function< void(int,int,const char*) > _updateFunction;
    std::function< const char*(int,int) > _emptyValueFunction;
};

#endif /* end of include guard: SQLITERANGER_USA0FKBB */


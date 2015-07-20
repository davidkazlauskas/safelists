#ifndef ASYNCSQLITE_F4E1YMYY
#define ASYNCSQLITE_F4E1YMYY

#include <LuaPlumbing/plumbing.hpp>

#define DUMMY_STRUCT(name)  \
    struct name { template <class Any> name(Any&&) {} };

namespace SafeLists {

struct AsyncSqlite {

    // Turn off and close database
    DUMMY_STRUCT(Shutdown);

    typedef std::function< void(int,const char**,const char**) > SqliteCallbackSimple;
    // query statement
    // Signature:
    // < Execute, const char* sql >
    // < Execute, const char* sql, SqliteCallbackSimple >
    DUMMY_STRUCT(Execute);

    static StrongMsgPtr createNew(const char* name);
};

}

#endif /* end of include guard: ASYNCSQLITE_F4E1YMYY */

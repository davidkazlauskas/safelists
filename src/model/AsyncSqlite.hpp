#ifndef ASYNCSQLITE_F4E1YMYY
#define ASYNCSQLITE_F4E1YMYY

#include <LuaPlumbing/plumbing.hpp>

#define DUMMY_STRUCT(name)  \
    struct name { template <class Any> name(Any&&) {} };

namespace SafeLists {

struct AsyncSqlite {

    // Turn off and close database
    DUMMY_STRUCT(Shutdown);

    // query statement
    // Signature:
    // < Execute, std::function< void(int,const char**,const char**) > >
    DUMMY_STRUCT(Execute);

    static StrongMsgPtr createNew(const char* name);
};

}

#endif /* end of include guard: ASYNCSQLITE_F4E1YMYY */

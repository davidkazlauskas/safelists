#ifndef ASYNCSQLITE_F4E1YMYY
#define ASYNCSQLITE_F4E1YMYY

#include <LuaPlumbing/plumbing.hpp>

#define DUMMY_STRUCT(name)  \
    struct name { template <class Any> name(Any&&) {} };

namespace SafeLists {

struct AsyncSqlite {

    DUMMY_STRUCT(Shutdown);

    static StrongMsgPtr createNew(const char* name);
};

}

#endif /* end of include guard: ASYNCSQLITE_F4E1YMYY */

#ifndef ASYNCSQLITE_F4E1YMYY
#define ASYNCSQLITE_F4E1YMYY

#include <LuaPlumbing/plumbing.hpp>
#include <util/DummyStruct.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct AsyncSqlite {

    // Turn off and close database
    DUMMY_STRUCT(Shutdown);

    typedef std::function< void(int,char**,char**) > SqliteCallbackSimple;
    // query statement
    // Signature:
    // < Execute, const char* sql >
    // < Execute, std::string >
    // < Execute, const char* sql, SqliteCallbackSimple >
    DUMMY_REG(Execute,"ASQL_Execute");

    // query something and receive table
    // snapshot
    // Signature:
    // <
    //     ExecuteOutSnapshot,
    //     std::string (query),
    //     std::vector< std::string > (headernames),
    //     TableSnapshot (output)
    // >
    DUMMY_STRUCT(ExecuteOutSnapshot);

    // perform arbitrary operation
    // Signature:
    // <
    //     std::function< void(sqlite3*) > // function to use
    // >
    DUMMY_STRUCT(ArbitraryOperation);

    // query something and receive single
    // number from first row and first column
    // Signature: <
    //     OutSingleNum,
    //     std::string (query),
    //     int (number),
    //     bool (did succeed)
    // >
    DUMMY_STRUCT(OutSingleNum);

    // query this message to ensure you're
    // performing action after another
    // using templatious virtual pack wait
    // trait
    DUMMY_STRUCT(DummyWait);

    static StrongMsgPtr createNew(const char* name);
};

}

#endif /* end of include guard: ASYNCSQLITE_F4E1YMYY */

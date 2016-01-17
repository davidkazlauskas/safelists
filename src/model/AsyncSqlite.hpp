#ifndef ASYNCSQLITE_F4E1YMYY
#define ASYNCSQLITE_F4E1YMYY

#include <LuaPlumbing/plumbing.hpp>
#include <util/DummyStruct.hpp>
#include <util/AutoReg.hpp>

namespace SafeLists {

struct AsyncSqlite {

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
    DUMMY_REG(OutSingleNum,"ASQL_OutSingleNum");

    // query something and receive single
    // row result separated by '|' pipes.
    // Signature: <
    //     OutSingleNum,
    //     std::string (query),
    //     std::string (output),
    //     bool (did succeed)
    // >
    DUMMY_REG(OutSingleRow,"ASQL_OutSingleRow");

    // execute statement and receive affected
    // rows number.
    // Signature: <
    //     OutAffected,
    //     std::string (query),
    //     int (affected rows)
    // >
    DUMMY_REG(OutAffected,"ASQL_OutAffected");

    // query this message to ensure you're
    // performing action after another
    // using templatious virtual pack wait
    // trait
    DUMMY_STRUCT(DummyWait);

    // Turn off and close database,
    // should be issued only by
    // AsyncSqliteProxy
    DUMMY_REG(Shutdown,"ASQL_Shutdown");

    // Check if connection is already closed
    // Signature:
    // < IsDead, bool (out) >
    DUMMY_REG(IsDead,"ASQL_IsDead");

    static StrongMsgPtr createNew(const char* name);
};

}

#endif /* end of include guard: ASYNCSQLITE_F4E1YMYY */

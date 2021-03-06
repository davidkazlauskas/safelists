
#define CATCH_CONFIG_MAIN
#include <templatious/FullPack.hpp>

#include "catch.hpp"

#include <model/IndexedListView.hpp>
#include <model/AsyncSqlite.hpp>
#include <model/TableSnapshot.hpp>
#include <model/SqliteRanger.hpp>

TEMPLATIOUS_TRIPLET_STD;

using namespace SafeLists;

TEST_CASE("model_basic_sqlite","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    {
        static const char* query =
            "SELECT Name FROM FRIENDS;";

        std::vector< std::string > friendos;

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*, std::function<void(int,char**,char**)>
        >(
            nullptr, query,
            [&](int cnt,char** values,char** headers) {
                SA::add(friendos,values[0]);
            }
        );
        msg->message(pack);

        pack->wait();

        SM::sort( friendos );

        REQUIRE( friendos[0] == "Jim" );
        REQUIRE( friendos[1] == "Rebecca" );
        REQUIRE( friendos[2] == "Robert" );
        REQUIRE( friendos[3] == "Roger" );
        REQUIRE( friendos[4] == "Tom" );
    }
}

TEST_CASE("model_table_snapshot","[model]") {
    const char* names[] = {"first name","last name"};
    TableSnapshotBuilder bld(2,names);

    bld.setValue(0,"mickey");
    bld.setValue(1,"mouse");
    bld.commitRow();

    bld.setValue(0,"some");
    bld.setValue(1,"name");
    bld.commitRow();

    auto snap = bld.getSnapshot();

    std::stringstream ss;

    snap.traverse(
        [&](int row,int column,const char* value,const char* header) {
            ss << row << column << value << header;
            return true;
        });

    auto outStr = ss.str();
    const char* expected =
        "00mickeyfirst name01mouselast name"
        "10somefirst name11namelast name";
    REQUIRE( outStr == expected );
}

TEST_CASE("model_table_snapshot_no_write","[model]") {
    const char* names[] = {"first name","last name"};
    TableSnapshotBuilder bld(2,names);

    bld.setValue(0,"mickey");
    //bld.setValue(1,"mouse");
    bld.commitRow();

    //bld.setValue(0,"some");
    bld.setValue(1,"name");
    bld.commitRow();

    auto snap = bld.getSnapshot();

    std::stringstream ss;
    int countDown = 3;

    snap.traverse(
        [&](int row,int column,const char* value,const char* header) {
            ss << row << column << value << header;
            --countDown;
            return countDown > 0;
        });

    auto outStr = ss.str();
    const char* expected =
        "00mickeyfirst name01[EMPTY]last name"
        "10[EMPTY]first name";
    REQUIRE( outStr == expected );
}

TEST_CASE("model_sqlite_snapshot","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    {
        static const char* query =
            "SELECT Name,Id FROM FRIENDS ORDER BY Name ASC;";

        std::vector< std::vector< std::string > > outMatrixVal;

        outMatrixVal.resize(5);
        TEMPLATIOUS_FOREACH(auto& i,outMatrixVal) {
            i.resize(2);
        }

        auto outMatrixHead = outMatrixVal;

        std::vector< std::string > headers;
        SA::add(headers,"The name","The id");
        auto pack = SF::vpackPtrCustom<
             templatious::VPACK_WAIT,
             AsyncSqlite::ExecuteOutSnapshot,
             std::string,
             std::vector< std::string >,
             TableSnapshot
        >(nullptr, query, std::move(headers), TableSnapshot());

        msg->message(pack);
        pack->wait();

        auto& snapRef = pack->fGet<3>();

        snapRef.traverse(
            [&](int row,int col,const char* val,const char* head) {
                outMatrixVal[row][col] = val;
                outMatrixHead[row][col] = head;
                return true;
            }
        );

        REQUIRE( outMatrixVal[0][0] == "Jim" );
        REQUIRE( outMatrixVal[0][1] == "3" );
        REQUIRE( outMatrixVal[1][0] == "Rebecca" );
        REQUIRE( outMatrixVal[1][1] == "2" );
        REQUIRE( outMatrixVal[2][0] == "Robert" );
        REQUIRE( outMatrixVal[2][1] == "5" );
        REQUIRE( outMatrixVal[3][0] == "Roger" );
        REQUIRE( outMatrixVal[3][1] == "4" );
        REQUIRE( outMatrixVal[4][0] == "Tom" );
        REQUIRE( outMatrixVal[4][1] == "1" );

        REQUIRE( outMatrixHead[0][0] == "The name" );
        REQUIRE( outMatrixHead[0][1] == "The id" );
        REQUIRE( outMatrixHead[1][0] == "The name" );
        REQUIRE( outMatrixHead[1][1] == "The id" );
        REQUIRE( outMatrixHead[2][0] == "The name" );
        REQUIRE( outMatrixHead[2][1] == "The id" );
        REQUIRE( outMatrixHead[3][0] == "The name" );
        REQUIRE( outMatrixHead[3][1] == "The id" );
        REQUIRE( outMatrixHead[4][0] == "The name" );
        REQUIRE( outMatrixHead[4][1] == "The id" );
    }
}

//const std::weak_ptr< Messageable >& asyncSqlite,
//const char* query,
//int columnCount,
//const UpdateFunction& updateFunction,
//const EmptyFunction& emptyFunction

void asyncSqliteWait(const std::shared_ptr< Messageable >& msg) {
    auto waitMsg = SF::vpackPtrCustom<
        templatious::VPACK_WAIT, AsyncSqlite::DummyWait >(nullptr);
    msg->message(waitMsg);
    waitMsg->wait();
}

TEST_CASE("model_sqlite_ranger","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    std::stringstream ss;
    auto sharedRanger = SqliteRanger::makeRanger(
        msg,
        "SELECT Id, Name FROM Friends LIMIT %d OFFSET %d;",
        "SELECT COUNT(*) FROM Friends;",
        2,
        [&ss](int row,int column,const char* value) {
            ss << "[" << row << ";" << column << "] -> '"
               << value << "'|";
        },
        [](int row,int column,std::string& out) {
            out = "[empty]";
        }
    );

    sharedRanger->setRange(1,3);
    asyncSqliteWait(msg);
    sharedRanger->process();

    auto str = ss.str();
    const char* EXPECTED =
        "[0;0] -> '2'|"
        "[0;1] -> 'Rebecca'|"
        "[1;0] -> '3'|"
        "[1;1] -> 'Jim'|"
    ;
    std::string expStr = EXPECTED;

    REQUIRE( str == expStr.c_str() );

    sharedRanger->setRange(2,5);
    asyncSqliteWait(msg);
    sharedRanger->process();

    const char* EXPECTED_2 =
        "[0;0] -> '3'|"
        "[0;1] -> 'Jim'|"
        "[1;0] -> '4'|"
        "[1;1] -> 'Roger'|"
        "[2;0] -> '5'|"
        "[2;1] -> 'Robert'|"
    ;

    expStr += EXPECTED_2;
    str = ss.str();

    REQUIRE( str == expStr );
}

TEST_CASE("model_sqlite_ranger_count","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    auto sharedRanger = SqliteRanger::makeRanger(
        msg,
        "SELECT Id, Name FROM Friends LIMIT %d OFFSET %d;",
        "SELECT COUNT(*) FROM Friends;",
        2,
        [](int row,int column,const char* value) {
        },
        [](int row,int column,std::string& out) {
        }
    );
    sharedRanger->updateRows();
    sharedRanger->waitRows();

    REQUIRE( 5 == sharedRanger->numRows() );
    REQUIRE( 2 == sharedRanger->numColumns() );
}

TEST_CASE("model_async_sqlite_single_row","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    {
        static const char* query =
            "SELECT Name,Id FROM FRIENDS LIMIT 1;";

        auto pack = SF::vpackPtrCustom<
            templatious::VPACK_WAIT,
            AsyncSqlite::OutSingleRow,
            const std::string,
            std::string,
            bool
        >(nullptr,query,"",false);
        msg->message(pack);
        pack->wait();

        REQUIRE( pack->fGet<3>() );
        REQUIRE( pack->fGet<2>() == "Tom|1" );
    }
}

TEST_CASE("model_async_sqlite_single_num","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        static const char* query =
            "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);"
            "INSERT INTO Friends(Name) VALUES ('Tom');"
            "INSERT INTO Friends(Name) VALUES ('Rebecca');"
            "INSERT INTO Friends(Name) VALUES ('Jim');"
            "INSERT INTO Friends(Name) VALUES ('Roger');"
            "INSERT INTO Friends(Name) VALUES ('Robert');";

        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, query
        );
        msg->message(pack);

        pack->wait();
    }

    {
        static const char* query =
            "SELECT COUNT(*) FROM FRIENDS;";

        auto pack = SF::vpackPtrCustom<
            templatious::VPACK_WAIT,
            AsyncSqlite::OutSingleNum,
            const std::string,
            int,
            bool
        >(nullptr,query,-1,false);
        msg->message(pack);
        pack->wait();

        REQUIRE( pack->fGet<3>() );
        REQUIRE( pack->fGet<2>() == 5 );
    }
}
